//
//
//      Copyright (C) EMC Corporation, 2004
//      All rights reserved.
//      Licensed material - Property of EMC Corporation.
//
//
//  Module:
//
//      K10CommonTrace.h
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

#ifndef K10_COMMON_TRACE_H
#define K10_COMMON_TRACE_H

//
// Pull in our private header file that contains all of our support macros and
// definitions.
//
#pragma warning( disable : 4162 )
#include <ntddk.h>
#include "k10diskdriveradmin.h"
#include "flare_export_ioctls.h"
#include "flare_ioctls.h"
#include "K10CommonTracePrivate.h"

//*****************************************************************************
//*****************************************************************************
// IRP_MJ_CREATE
//*****************************************************************************
//*****************************************************************************

//
// Macro:       K10_COMMON_TRACE_IRP_MJ_CREATE_RECEPTION
//
// Description: Used upon operation reception.  If the LUN is known, use the
//              "BY_LUN" version of this macro.
//
// Parameters:  m_component
//                  The identifier of the caller.
//
//              m_device_name
//                  The string that represents the device name.
//
//              m_device_object
//                  The pointer to the device object.
//
//              m_open_count
//                  The current open count for the device.
//

#define K10_COMMON_TRACE_IRP_MJ_CREATE_RECEPTION( m_component, \
                                                  m_device_name, \
                                                  m_device_object, \
                                                  m_open_count ) \
    K10_COMMON_TRACE_RECEPTION_OPEN_COUNT( \
        K10_COMMON_TRACE_IRP_MJ_CREATE_STR, \
        m_component, \
        m_device_name, \
        m_device_object, \
        K10_COMMON_TRACE_UNUSED_PARAMETER, \
        m_open_count, \
        K10_COMMON_TRACE_UNUSED_PARAMETER )

//
// Macro:       K10_COMMON_TRACE_IRP_MJ_CREATE_RECEPTION_TOTAL_OPEN
//
// Description: Used upon operation reception.  If the LUN is known, use the
//              "BY_LUN" version of this macro.  This macro must be used by
//              drivers that unify devices into device graphs to trace the
//              total open count for their devices.
//
// Parameters:  m_component
//                  The identifier of the caller.
//
//              m_device_name
//                  The string that represents the device name.
//
//              m_device_object
//                  The pointer to the device object.
//
//              m_total_open_count
//                  The current total open count for the devices in the device
//                  graph managed by this component.
//

#define K10_COMMON_TRACE_IRP_MJ_CREATE_RECEPTION_TOTAL_OPEN( m_component, \
                                                             m_device_name, \
                                                             m_device_object, \
                                                             m_total_open_count ) \
    K10_COMMON_TRACE_RECEPTION_OPEN_COUNT( \
        K10_COMMON_TRACE_IRP_MJ_CREATE_STR, \
        m_component, \
        m_device_name, \
        m_device_object, \
        K10_COMMON_TRACE_UNUSED_PARAMETER, \
        K10_COMMON_TRACE_UNUSED_PARAMETER, \
        m_total_open_count )

//
// Macro:       K10_COMMON_TRACE_IRP_MJ_CREATE_RECEPTION_BY_LUN
//
// Description: Used upon operation reception.
//
// Parameters:  m_component
//                  The identifier of the caller.
//
//              m_device_name
//                  The string that represents the device name.
//
//              m_device_object
//                  The pointer to the device object.
//
//              m_lun
//                  The LUN that corresponds to this device.
//
//              m_open_count
//                  The current open count for the device.
//

#define K10_COMMON_TRACE_IRP_MJ_CREATE_RECEPTION_BY_LUN( m_component, \
                                                         m_device_name, \
                                                         m_device_object, \
                                                         m_lun, \
                                                         m_open_count ) \
    K10_COMMON_TRACE_RECEPTION_OPEN_COUNT( \
        K10_COMMON_TRACE_IRP_MJ_CREATE_STR, \
        m_component, \
        m_device_name, \
        m_device_object, \
        m_lun, \
        m_open_count, \
        K10_COMMON_TRACE_UNUSED_PARAMETER )

//
// Macro:       K10_COMMON_TRACE_IRP_MJ_CREATE_RECEPTION_TOTAL_OPEN_BY_LUN
//
// Description: Used upon operation reception.  This macro must be used by
//              drivers that unify devices into device graphs to trace the
//              total open count for their devices.
//
// Parameters:  m_component
//                  The identifier of the caller.
//
//              m_device_name
//                  The string that represents the device name.
//
//              m_device_object
//                  The pointer to the device object.
//
//              m_total_open_count
//                  The current total open count for the devices in the device
//                  graph managed by this component.
//

#define K10_COMMON_TRACE_IRP_MJ_CREATE_RECEPTION_TOTAL_OPEN_BY_LUN( m_component, \
                                                                    m_device_name, \
                                                                    m_device_object, \
                                                                    m_lun, \
                                                                    m_total_open_count ) \
    K10_COMMON_TRACE_RECEPTION_OPEN_COUNT( \
        K10_COMMON_TRACE_IRP_MJ_CREATE_STR, \
        m_component, \
        m_device_name, \
        m_device_object, \
        m_lun, \
        K10_COMMON_TRACE_UNUSED_PARAMETER, \
        m_total_open_count )

//
// Macro:       K10_COMMON_TRACE_IRP_MJ_CREATE_ACTUALLY_OPENING
//
// Description: Used when the device below is opened.  If the LUN is known,
//              use the "BY_LUN" version of this macro.
//
// Parameters:  m_component
//                  The identifier of the caller.
//
//              m_consumed_device_name
//                  The string that represents the consumed device name.
//
//              m_exported_device_object
//                  The pointer to the exported device object.
//

#define K10_COMMON_TRACE_IRP_MJ_CREATE_ACTUALLY_OPENING( m_component, \
                                                       m_consumed_device_name, \
                                                       m_exported_device_object ) \
        K10_COMMON_TRACE_DEVICE_WITH_FORMAT_PARAM( K10_COMMON_TRACE_IRP_MJ_CREATE_STR " Actually Opening", \
                                                   m_component, \
                                                   m_consumed_device_name, \
                                                   m_exported_device_object, \
                                                   K10_COMMON_TRACE_UNUSED_PARAMETER, \
                                                   "%s", \
                                                   "" )

//
// Macro:       K10_COMMON_TRACE_IRP_MJ_CREATE_ACTUALLY_OPENING_BY_LUN
//
// Description: Used when the device below is opened.
//
// Parameters:  m_component
//                  The identifier of the caller.
//
//              m_consumed_device_name
//                  The string that represents the consumed device name.
//
//              m_exported_device_object
//                  The pointer to the exported device object.
//
//              m_lun
//                  The LUN that corresponds to this device.
//

#define K10_COMMON_TRACE_IRP_MJ_CREATE_ACTUALLY_OPENING_BY_LUN( m_component, \
                                                              m_consumed_device_name, \
                                                              m_exported_device_object, \
                                                              m_lun ) \
        K10_COMMON_TRACE_DEVICE_WITH_FORMAT_PARAM( K10_COMMON_TRACE_IRP_MJ_CREATE_STR " Actually Opening", \
                                                    m_component, \
                                                    m_consumed_device_name, \
                                                    m_exported_device_object, \
                                                    m_lun, \
                                                    "%s", \
                                                    "" )

//
// Macro:       K10_COMMON_TRACE_IRP_MJ_CREATE_ERROR
//
// Description: Used when an error is encountered during the operation.  A
//              unique error code must be used to identify the cause of the
//              error.  If the LUN is known, use the "BY_LUN" version of this
//              macro.
//
// Parameters:  m_component
//                  The identifier of the caller.
//
//              m_device_name
//                  The string that represents the device name.
//
//              m_error_code
//                  The error code that was encountered that needs to be
//                  displayed.
//

#define K10_COMMON_TRACE_IRP_MJ_CREATE_ERROR( m_component, \
                                              m_device_name, \
                                              m_error_code ) \
        K10_COMMON_TRACE_DEVICE_WITH_FORMAT_PARAM( K10_COMMON_TRACE_IRP_MJ_CREATE_STR " Error", \
                                                   m_component, \
                                                   m_device_name, \
                                                   m_error_code, \
                                                   K10_COMMON_TRACE_UNUSED_PARAMETER, \
                                                   "%s", \
                                                   "" )

//
// Macro:       K10_COMMON_TRACE_IRP_MJ_CREATE_ERROR_BY_LUN
//
// Description: Used when an error is encountered during the operation.  A
//              unique error code must be used to identify the cause of the
//              error.
//
// Parameters:  m_component
//                  The identifier of the caller.
//
//              m_device_name
//                  The string that represents the device name.
//
//              m_lun
//                  The LUN that corresponds to this device.
//
//              m_error_code
//                  The error code that was encountered that needs to be
//                  displayed.
//
#define K10_COMMON_TRACE_IRP_MJ_CREATE_ERROR_BY_LUN( m_component, \
                                                     m_device_name, \
                                                     m_lun, \
                                                     m_error_code ) \
        K10_COMMON_TRACE_DEVICE_WITH_FORMAT_PARAM( K10_COMMON_TRACE_IRP_MJ_CREATE_STR " Error", \
                                                   m_component, \
                                                   m_device_name, \
                                                   m_error_code, \
                                                   m_lun, \
                                                   "%s", \
                                                   "" )

//
// Macro:       K10_COMMON_TRACE_IRP_MJ_CREATE_MSG
//
// Description: Used to display additional information on the operation.  
//              If the LUN is known, use the "BY_LUN" version of this macro.
//
// Parameters:  m_component
//                  The identifier of the caller.
//
//              m_device_object
//                  The pointer to the device object.
//
//              m_msg_string
//                  A msg string to display to the user.
//
//              m_status_code
//                  A unique status code the corresponds to the msg string.
//                  Must be 0 if there is no appropriate status code.
//

#define K10_COMMON_TRACE_IRP_MJ_CREATE_MSG( m_component, \
                                            m_device_object, \
                                            m_msg_string, \
                                            m_status_code ) \
        K10_COMMON_TRACE_DEVICE_WITH_FORMAT_PARAM( K10_COMMON_TRACE_IRP_MJ_CREATE_STR, \
                                                   m_component, \
                                                   "", \
                                                   m_device_object, \
                                                   K10_COMMON_TRACE_UNUSED_PARAMETER, \
                                                   "\"" m_msg_string "\"" " 0x%x", \
                                                   m_status_code )

//
// Macro:       K10_COMMON_TRACE_IRP_MJ_CREATE_MSG_BY_LUN
//
// Description: Used to display additional information on the operation.  
//
// Parameters:  m_component
//                  The identifier of the caller.
//
//              m_device_object
//                  The pointer to the device object.
//
//              m_lun
//                  The LUN that corresponds to this device.
//
//              m_msg_string
//                  A msg string to display to the user.
//
//              m_status_code
//                  A unique status code the corresponds to the msg string.
//                  Must be 0 if there is no appropriate status code.
//

#define K10_COMMON_TRACE_IRP_MJ_CREATE_MSG_BY_LUN( m_component, \
                                                   m_device_object, \
                                                   m_lun, \
                                                   m_msg_string, \
                                                   m_status_code ) \
        K10_COMMON_TRACE_DEVICE_WITH_FORMAT_PARAM( K10_COMMON_TRACE_IRP_MJ_CREATE_STR, \
                                                   m_component, \
                                                   "", \
                                                   m_device_object, \
                                                   m_lun, \
                                                   "\"" m_msg_string "\"" " 0x%x", \
                                                   m_status_code )                                                   

//*****************************************************************************
//*****************************************************************************
// IRP_MJ_CLOSE
//*****************************************************************************
//*****************************************************************************

//
// Macro:       K10_COMMON_TRACE_IRP_MJ_CLOSE_RECEPTION
//
// Description: Used upon operation reception.  If the LUN is known, use the
//              "BY_LUN" version of this macro.
//
// Parameters:  m_component
//                  The identifier of the caller.
//
//              m_device_name
//                  The string that represents the device name.
//
//              m_device_object
//                  The pointer to the device object.
//
//              m_open_count
//                  The current open count for the device.
//

#define K10_COMMON_TRACE_IRP_MJ_CLOSE_RECEPTION( m_component, \
                                                 m_device_name, \
                                                 m_device_object, \
                                                 m_open_count ) \
    K10_COMMON_TRACE_RECEPTION_OPEN_COUNT( \
        K10_COMMON_TRACE_IRP_MJ_CLOSE_STR, \
        m_component, \
        m_device_name, \
        m_device_object, \
        K10_COMMON_TRACE_UNUSED_PARAMETER, \
        m_open_count, \
        K10_COMMON_TRACE_UNUSED_PARAMETER )

//
// Macro:       K10_COMMON_TRACE_IRP_MJ_CLOSE_RECEPTION_TOTAL_OPEN
//
// Description: Used upon operation reception.  If the LUN is known, use the
//              "BY_LUN" version of this macro.  This macro must be used by
//              drivers that unify devices into device graphs to trace the
//              total open count for their devices.
//
// Parameters:  m_component
//                  The identifier of the caller.
//
//              m_device_name
//                  The string that represents the device name.
//
//              m_device_object
//                  The pointer to the device object.
//
//              m_total_open_count
//                  The current total open count for the devices in the device
//                  graph managed by this component.
//

#define K10_COMMON_TRACE_IRP_MJ_CLOSE_RECEPTION_TOTAL_OPEN( m_component, \
                                                            m_device_name, \
                                                            m_device_object, \
                                                            m_total_open_count ) \
    K10_COMMON_TRACE_RECEPTION_OPEN_COUNT( \
        K10_COMMON_TRACE_IRP_MJ_CLOSE_STR, \
        m_component, \
        m_device_name, \
        m_device_object, \
        K10_COMMON_TRACE_UNUSED_PARAMETER, \
        K10_COMMON_TRACE_UNUSED_PARAMETER, \
        m_total_open_count )

//
// Macro:       K10_COMMON_TRACE_IRP_MJ_CLOSE_RECEPTION_BY_LUN
//
// Description: Used upon operation reception.
//
// Parameters:  m_component
//                  The identifier of the caller.
//
//              m_device_name
//                  The string that represents the device name.
//
//              m_device_object
//                  The pointer to the device object.
//
//              m_lun
//                  The LUN that corresponds to this device.
//
//              m_open_count
//                  The current open count for the device.
//

#define K10_COMMON_TRACE_IRP_MJ_CLOSE_RECEPTION_BY_LUN( m_component, \
                                                        m_device_name, \
                                                        m_device_object, \
                                                        m_lun, \
                                                        m_open_count ) \
    K10_COMMON_TRACE_RECEPTION_OPEN_COUNT( \
        K10_COMMON_TRACE_IRP_MJ_CLOSE_STR, \
        m_component, \
        m_device_name, \
        m_device_object, \
        m_lun, \
        m_open_count, \
        K10_COMMON_TRACE_UNUSED_PARAMETER )

//
// Macro:       K10_COMMON_TRACE_IRP_MJ_CLOSE_RECEPTION_TOTAL_OPEN_BY_LUN
//
// Description: Used upon operation reception.  This macro must be used by
//              drivers that unify devices into device graphs to trace the
//              total open count for their devices.
//
// Parameters:  m_component
//                  The identifier of the caller.
//
//              m_device_name
//                  The string that represents the device name.
//
//              m_device_object
//                  The pointer to the device object.
//
//              m_lun
//                  The LUN that corresponds to this device.
//
//              m_total_open_count
//                  The current total open count for the devices in the device
//                  graph managed by this component.
//

#define K10_COMMON_TRACE_IRP_MJ_CLOSE_RECEPTION_TOTAL_OPEN_BY_LUN( m_component, \
                                                                   m_device_name, \
                                                                   m_device_object, \
                                                                   m_lun, \
                                                                   m_total_open_count ) \
    K10_COMMON_TRACE_RECEPTION_OPEN_COUNT( \
        K10_COMMON_TRACE_IRP_MJ_CLOSE_STR, \
        m_component, \
        m_device_name, \
        m_device_object, \
        m_lun, \
        K10_COMMON_TRACE_UNUSED_PARAMETER, \
        m_total_open_count )

//
// Macro:       K10_COMMON_TRACE_IRP_MJ_CLOSE_ACTUALLY_OPENING
//
// Description: Used when the device below is opened.  If the LUN is known,
//              use the "BY_LUN" version of this macro.
//
// Parameters:  m_component
//                  The identifier of the caller.
//
//              m_consumed_device_name
//                  The string that represents the consumed device name.
//
//              m_exported_device_object
//                  The pointer to the exported device object.
//

#define K10_COMMON_TRACE_IRP_MJ_CLOSE_ACTUALLY_OPENING( m_component, \
                                                       m_consumed_device_name, \
                                                       m_exported_device_object ) \
        K10_COMMON_TRACE_DEVICE_WITH_FORMAT_PARAM( K10_COMMON_TRACE_IRP_MJ_CLOSE_STR " Actually Opening", \
                                                   m_component, \
                                                   m_consumed_device_name, \
                                                   m_exported_device_object, \
                                                   K10_COMMON_TRACE_UNUSED_PARAMETER, \
                                                   "%s", \
                                                   "" )

//
// Macro:       K10_COMMON_TRACE_IRP_MJ_CLOSE_ACTUALLY_OPENING_BY_LUN
//
// Description: Used when the device below is opened.
//
// Parameters:  m_component
//                  The identifier of the caller.
//
//              m_consumed_device_name
//                  The string that represents the consumed device name.
//
//              m_exported_device_object
//                  The pointer to the exported device object.
//
//              m_lun
//                  The LUN that corresponds to this device.
//

#define K10_COMMON_TRACE_IRP_MJ_CLOSE_ACTUALLY_OPENING_BY_LUN( m_component, \
                                                              m_consumed_device_name, \
                                                              m_exported_device_object, \
                                                              m_lun ) \
        K10_COMMON_TRACE_DEVICE_WITH_FORMAT_PARAM( K10_COMMON_TRACE_IRP_MJ_CLOSE_STR " Actually Opening", \
                                                    m_component, \
                                                    m_consumed_device_name, \
                                                    m_exported_device_object, \
                                                    m_lun, \
                                                    "%s", \
                                                    "" )

//
// Macro:       K10_COMMON_TRACE_IRP_MJ_CLOSE_ERROR
//
// Description: Used when an error is encountered during the operation.  A
//              unique error code must be used to identify the cause of the
//              error.  If the LUN is known, use the "BY_LUN" version of this
//              macro.
//
// Parameters:  m_component
//                  The identifier of the caller.
//
//              m_device_name
//                  The string that represents the device name.
//
//              m_error_code
//                  The error code that was encountered that needs to be
//                  displayed.
//

#define K10_COMMON_TRACE_IRP_MJ_CLOSE_ERROR( m_component, \
                                              m_device_name, \
                                              m_error_code ) \
        K10_COMMON_TRACE_DEVICE_WITH_FORMAT_PARAM( K10_COMMON_TRACE_IRP_MJ_CLOSE_STR " Error", \
                                                   m_component, \
                                                   m_device_name, \
                                                   m_error_code, \
                                                   K10_COMMON_TRACE_UNUSED_PARAMETER, \
                                                   "%s", \
                                                   "" )

//
// Macro:       K10_COMMON_TRACE_IRP_MJ_CLOSE_ERROR_BY_LUN
//
// Description: Used when an error is encountered during the operation.  A
//              unique error code must be used to identify the cause of the
//              error.
//
// Parameters:  m_component
//                  The identifier of the caller.
//
//              m_device_name
//                  The string that represents the device name.
//
//              m_lun
//                  The LUN that corresponds to this device.
//
//              m_error_code
//                  The error code that was encountered that needs to be
//                  displayed.
//

#define K10_COMMON_TRACE_IRP_MJ_CLOSE_ERROR_BY_LUN( m_component, \
                                                     m_device_name, \
                                                     m_lun, \
                                                     m_error_code ) \
        K10_COMMON_TRACE_DEVICE_WITH_FORMAT_PARAM( K10_COMMON_TRACE_IRP_MJ_CLOSE_STR " Error", \
                                                   m_component, \
                                                   m_device_name, \
                                                   m_error_code, \
                                                   m_lun, \
                                                   "%s", \
                                                   "" )

//
// Macro:       K10_COMMON_TRACE_IRP_MJ_CLOSE_MSG
//
// Description: Used to display additional information on the operation.  
//              If the LUN is known, use the "BY_LUN" version of this macro.
//
// Parameters:  m_component
//                  The identifier of the caller.
//
//              m_device_object
//                  The pointer to the device object.
//
//              m_msg_string
//                  A msg string to display to the user.
//
//              m_status_code
//                  A unique status code the corresponds to the msg string.
//                  Must be 0 if there is no appropriate status code.
//

#define K10_COMMON_TRACE_IRP_MJ_CLOSE_MSG( m_component, \
                                           m_device_object, \
                                           m_msg_string, \
                                           m_status_code ) \
        K10_COMMON_TRACE_DEVICE_WITH_FORMAT_PARAM( K10_COMMON_TRACE_IRP_MJ_CLOSE_STR, \
                                                   m_component, \
                                                   "", \
                                                   m_device_object, \
                                                   K10_COMMON_TRACE_UNUSED_PARAMETER, \
                                                   "\"" m_msg_string "\"" " 0x%x", \
                                                   m_status_code )

//
// Macro:       K10_COMMON_TRACE_IRP_MJ_CLOSE_MSG_BY_LUN
//
// Description: Used to display additional information on the operation.  
//
// Parameters:  m_component
//                  The identifier of the caller.
//
//              m_device_object
//                  The pointer to the device object.
//
//              m_lun
//                  The LUN that corresponds to this device.
//
//              m_msg_string
//                  A msg string to display to the user.
//
//              m_status_code
//                  A unique status code the corresponds to the msg string.
//                  Must be 0 if there is no appropriate status code.
//

#define K10_COMMON_TRACE_IRP_MJ_CLOSE_MSG_BY_LUN( m_component, \
                                                  m_device_object, \
                                                  m_lun, \
                                                  m_msg_string, \
                                                  m_status_code ) \
        K10_COMMON_TRACE_DEVICE_WITH_FORMAT_PARAM( K10_COMMON_TRACE_IRP_MJ_CLOSE_STR, \
                                                   m_component, \
                                                   "", \
                                                   m_device_object, \
                                                   m_lun, \
                                                   "\"" m_msg_string "\"" " 0x%x", \
                                                   m_status_code )

//*****************************************************************************
//*****************************************************************************
// IOCTL_FLARE_TRESPASS_QUERY
//*****************************************************************************
//*****************************************************************************

//
// Macro:       K10_COMMON_TRACE_IOCTL_FLARE_TRESPASS_QUERY_RECEPTION
//
// Description: Used upon operation reception.  If the LUN is known, use the
//              "BY_LUN" version of this macro.
//
// Parameters:  m_component
//                  The identifier of the caller.
//
//              m_device_name
//                  The string that represents the device name.
//
//              m_device_object
//                  The pointer to the device object.
//

#define K10_COMMON_TRACE_IOCTL_FLARE_TRESPASS_QUERY_RECEPTION( m_component, \
                                                               m_device_name, \
                                                               m_device_object ) \
        K10_COMMON_TRACE_DEVICE_WITH_FORMAT_PARAM( K10_COMMON_TRACE_IOCTL_FLARE_TRESPASS_QUERY_STR, \
                                                   m_component, \
                                                   m_device_name, \
                                                   m_device_object, \
                                                   K10_COMMON_TRACE_UNUSED_PARAMETER, \
                                                   "%s", \
                                                   "Received" )

//
// Macro:       K10_COMMON_TRACE_IOCTL_FLARE_TRESPASS_QUERY_RECEPTION_BY_LUN
//
// Description: Used upon operation reception.  
//
// Parameters:  m_component
//                  The identifier of the caller.
//
//              m_device_name
//                  The string that represents the device name.
//
//              m_device_object
//                  The pointer to the device object.
//
//              m_lun
//                  The LUN that corresponds to this device.
//

#define K10_COMMON_TRACE_IOCTL_FLARE_TRESPASS_QUERY_RECEPTION_BY_LUN( m_component, \
                                                                      m_device_name, \
                                                                      m_device_object, \
                                                                      m_lun ) \
        K10_COMMON_TRACE_DEVICE_WITH_FORMAT_PARAM( K10_COMMON_TRACE_IOCTL_FLARE_TRESPASS_QUERY_STR, \
                                                   m_component, \
                                                   m_device_name, \
                                                   m_device_object, \
                                                   m_lun, \
                                                   "%s", \
                                                   "Received" )


//
// Macro:       K10_COMMON_TRACE_IOCTL_FLARE_TRESPASS_QUERY_REJECTED
//
// Description: Used on trespass query reject.  If the LUN is known, use the
//              "BY_LUN" version of this macro.
//
// Parameters:  m_component
//                  The identifier of the caller.
//
//              m_device_object
//                  The pointer to the device object.
//
//              m_reject_string
//                  A string that identifies the reason the query was
//                  rejected.
//
//              m_error_code
//                  A unique error code the corresponds to the reason for
//                  the rejection.
//

#define K10_COMMON_TRACE_IOCTL_FLARE_TRESPASS_QUERY_REJECTED( m_component, \
                                                              m_device_object, \
                                                              m_reject_string, \
                                                              m_error_code ) \
        K10_COMMON_TRACE_DEVICE_WITH_FORMAT_PARAM( K10_COMMON_TRACE_IOCTL_FLARE_TRESPASS_QUERY_STR, \
                                                   m_component, \
                                                   "", \
                                                   m_device_object, \
                                                   K10_COMMON_TRACE_UNUSED_PARAMETER, \
                                                   "Rejected " "\"" m_reject_string "\"" " 0x%x", \
                                                   m_error_code )

//
// Macro:       K10_COMMON_TRACE_IOCTL_FLARE_TRESPASS_QUERY_REJECTED_BY_LUN
//
// Description: Used on trespass query reject.  
//
// Parameters:  m_component
//                  The identifier of the caller.
//
//              m_device_object
//                  The pointer to the device object.
//
//              m_lun
//                  The LUN that corresponds to this device.
//
//              m_reject_string
//                  A string that identifies the reason the query was
//                  rejected.
//
//              m_error_code
//                  A unique error code the corresponds to the reason for
//                  the rejection.
//

#define K10_COMMON_TRACE_IOCTL_FLARE_TRESPASS_QUERY_REJECTED_BY_LUN( m_component, \
                                                                     m_device_object, \
                                                                     m_lun, \
                                                                     m_reject_string, \
                                                                     m_error_code ) \
        K10_COMMON_TRACE_DEVICE_WITH_FORMAT_PARAM( K10_COMMON_TRACE_IOCTL_FLARE_TRESPASS_QUERY_STR, \
                                                   m_component, \
                                                   "", \
                                                   m_device_object, \
                                                   m_lun, \
                                                   "Rejected " "\"" m_reject_string "\"" " 0x%x", \
                                                   m_error_code )

//
// Macro:       K10_COMMON_TRACE_IOCTL_FLARE_TRESPASS_QUERY_MSG
//
// Description: Used to display additional information on the operation.  
//              If the LUN is known, use the "BY_LUN" version of this macro.
//
// Parameters:  m_component
//                  The identifier of the caller.
//
//              m_device_object
//                  The pointer to the device object.
//
//              m_msg_string
//                  A msg string to display to the user.
//
//              m_status_code
//                  A unique status code the corresponds to the msg string.
//                  Must be 0 if there is no appropriate status code.
//

#define K10_COMMON_TRACE_IOCTL_FLARE_TRESPASS_QUERY_MSG( m_component, \
                                                         m_device_object, \
                                                         m_msg_string, \
                                                         m_status_code ) \
        K10_COMMON_TRACE_DEVICE_WITH_FORMAT_PARAM( K10_COMMON_TRACE_IOCTL_FLARE_TRESPASS_QUERY_STR, \
                                                   m_component, \
                                                   "", \
                                                   m_device_object, \
                                                   K10_COMMON_TRACE_UNUSED_PARAMETER, \
                                                   "\"" m_msg_string "\"" " 0x%x", \
                                                   m_status_code )

//
// Macro:       K10_COMMON_TRACE_IOCTL_FLARE_TRESPASS_QUERY_MSG_BY_LUN
//
// Description: Used to display additional information on the operation.  
//
// Parameters:  m_component
//                  The identifier of the caller.
//
//              m_device_object
//                  The pointer to the device object.
//
//              m_lun
//                  The LUN that corresponds to this device.
//
//              m_msg_string
//                  A msg string to display to the user.
//
//              m_status_code
//                  A unique status code the corresponds to the msg string.
//                  Must be 0 if there is no appropriate status code.
//

#define K10_COMMON_TRACE_IOCTL_FLARE_TRESPASS_QUERY_MSG_BY_LUN( m_component, \
                                                                m_device_object, \
                                                                m_lun, \
                                                                m_msg_string, \
                                                                m_status_code ) \
        K10_COMMON_TRACE_DEVICE_WITH_FORMAT_PARAM( K10_COMMON_TRACE_IOCTL_FLARE_TRESPASS_QUERY_STR, \
                                                   m_component, \
                                                   "", \
                                                   m_device_object, \
                                                   m_lun, \
                                                   "\"" m_msg_string "\"" " 0x%x", \
                                                   m_status_code )


//*****************************************************************************
//*****************************************************************************
// IOCTL_FLARE_TRESPASS_OWNERSHIP_LOSS
//*****************************************************************************
//*****************************************************************************

//
// Macro:       K10_COMMON_TRACE_IOCTL_FLARE_TRESPASS_OWNERSHIP_LOSS_RECEPTION
//
// Description: Used upon operation reception.  If the LUN is known, use the
//              "BY_LUN" version of this macro.
//
// Parameters:  m_component
//                  The identifier of the caller.
//
//              m_device_name
//                  The string that represents the device name.
//
//              m_device_object
//                  The pointer to the device object.
//

#define K10_COMMON_TRACE_IOCTL_FLARE_TRESPASS_OWNERSHIP_LOSS_RECEPTION( m_component, \
                                                                        m_device_name, \
                                                                        m_device_object ) \
        K10_COMMON_TRACE_DEVICE_WITH_FORMAT_PARAM( K10_COMMON_TRACE_IOCTL_FLARE_TRESPASS_OWNERSHIP_LOSS_STR, \
                                                   m_component, \
                                                   m_device_name, \
                                                   m_device_object, \
                                                   K10_COMMON_TRACE_UNUSED_PARAMETER, \
                                                   "%s", \
                                                   "Received" )

//
// Macro:       K10_COMMON_TRACE_IOCTL_FLARE_TRESPASS_OWNERSHIP_LOSS_RECEPTION_BY_LUN
//
// Description: Used upon operation reception.
//
// Parameters:  m_component
//                  The identifier of the caller.
//
//              m_device_name
//                  The string that represents the device name.
//
//              m_device_object
//                  The pointer to the device object.
//
//              m_lun
//                  The LUN that corresponds to this device.
//

#define K10_COMMON_TRACE_IOCTL_FLARE_TRESPASS_OWNERSHIP_LOSS_RECEPTION_BY_LUN( m_component, \
                                                                               m_device_name, \
                                                                               m_device_object, \
                                                                               m_lun ) \
        K10_COMMON_TRACE_DEVICE_WITH_FORMAT_PARAM( K10_COMMON_TRACE_IOCTL_FLARE_TRESPASS_OWNERSHIP_LOSS_STR, \
                                                   m_component, \
                                                   m_device_name, \
                                                   m_device_object, \
                                                   m_lun, \
                                                   "%s", \
                                                   "Received" )


//
// Macro:       K10_COMMON_TRACE_IOCTL_FLARE_TRESPASS_OWNERSHIP_LOSS_COMPLETION
//
// Description: Used upon operation reception.  If the LUN is known, use the
//              "BY_LUN" version of this macro.
//
// Parameters:  m_component
//                  The identifier of the caller.
//
//              m_device_name
//                  The string that represents the device name.
//
//              m_device_object
//                  The pointer to the device object.
//

#define K10_COMMON_TRACE_IOCTL_FLARE_TRESPASS_OWNERSHIP_LOSS_COMPLETION( m_component, \
                                                                         m_device_name, \
                                                                         m_device_object ) \
        K10_COMMON_TRACE_DEVICE_WITH_FORMAT_PARAM( K10_COMMON_TRACE_IOCTL_FLARE_TRESPASS_OWNERSHIP_LOSS_STR, \
                                                   m_component, \
                                                   m_device_name, \
                                                   m_device_object, \
                                                   K10_COMMON_TRACE_UNUSED_PARAMETER, \
                                                   "%s", \
                                                   "Complete" )

//
// Macro:       K10_COMMON_TRACE_IOCTL_FLARE_TRESPASS_OWNERSHIP_LOSS_COMPLETION_BY_LUN
//
// Description: Used upon operation reception.
//
// Parameters:  m_component
//                  The identifier of the caller.
//
//              m_device_name
//                  The string that represents the device name.
//
//              m_device_object
//                  The pointer to the device object.
//
//              m_lun
//                  The LUN that corresponds to this device.
//

#define K10_COMMON_TRACE_IOCTL_FLARE_TRESPASS_OWNERSHIP_LOSS_COMPLETION_BY_LUN( m_component, \
                                                                                m_device_name, \
                                                                                m_device_object, \
                                                                                m_lun ) \
        K10_COMMON_TRACE_DEVICE_WITH_FORMAT_PARAM( K10_COMMON_TRACE_IOCTL_FLARE_TRESPASS_OWNERSHIP_LOSS_STR, \
                                                   m_component, \
                                                   m_device_name, \
                                                   m_device_object, \
                                                   m_lun, \
                                                   "%s", \
                                                   "Complete" )

//
// Macro:       K10_COMMON_TRACE_IOCTL_FLARE_TRESPASS_OWNERSHIP_LOSS_ACTUALLY_LOSING
//
// Description: Used when the device below is opened.  If the LUN is known,
//              use the "BY_LUN" version of this macro.
//
// Parameters:  m_component
//                  The identifier of the caller.
//
//              m_consumed_device_name
//                  The string that represents the consumed device name.
//
//              m_exported_device_object
//                  The pointer to the exported device object.
//

#define K10_COMMON_TRACE_IOCTL_FLARE_TRESPASS_OWNERSHIP_LOSS_ACTUALLY_LOSING( m_component, \
                                                                              m_consumed_device_name, \
                                                                              m_exported_device_object ) \
        K10_COMMON_TRACE_DEVICE_WITH_FORMAT_PARAM( K10_COMMON_TRACE_IOCTL_FLARE_TRESPASS_OWNERSHIP_LOSS_STR " Actually Losing", \
                                                   m_component, \
                                                   m_consumed_device_name, \
                                                   m_exported_device_object, \
                                                   K10_COMMON_TRACE_UNUSED_PARAMETER, \
                                                   "%s", \
                                                   "" )

//
// Macro:       K10_COMMON_TRACE_IOCTL_FLARE_TRESPASS_OWNERSHIP_LOSS_ACTUALLY_LOSING_BY_LUN
//
// Description: Used when the device below is opened.
//
// Parameters:  m_component
//                  The identifier of the caller.
//
//              m_consumed_device_name
//                  The string that represents the consumed device name.
//
//              m_exported_device_object
//                  The pointer to the exported device object.
//
//              m_lun
//                  The LUN that corresponds to this device.
//

#define K10_COMMON_TRACE_IOCTL_FLARE_TRESPASS_OWNERSHIP_LOSS_ACTUALLY_LOSING_BY_LUN( m_component, \
                                                                                     m_consumed_device_name, \
                                                                                     m_exported_device_object, \
                                                                                     m_lun ) \
        K10_COMMON_TRACE_DEVICE_WITH_FORMAT_PARAM( K10_COMMON_TRACE_IOCTL_FLARE_TRESPASS_OWNERSHIP_LOSS_STR " Actually Losing", \
                                                    m_component, \
                                                    m_consumed_device_name, \
                                                    m_exported_device_object, \
                                                    m_lun, \
                                                    "%s", \
                                                    "" )

//
// Macro:       K10_COMMON_TRACE_IOCTL_FLARE_TRESPASS_OWNERSHIP_LOSS_MSG
//
// Description: Used to display additional information on the operation.  
//              If the LUN is known, use the "BY_LUN" version of this macro.
//
// Parameters:  m_component
//                  The identifier of the caller.
//
//              m_device_object
//                  The pointer to the device object.
//
//              m_msg_string
//                  A msg string to display to the user.
//
//              m_status_code
//                  A unique status code the corresponds to the msg string.
//                  Must be 0 if there is no appropriate status code.
//

#define K10_COMMON_TRACE_IOCTL_FLARE_TRESPASS_OWNERSHIP_LOSS_MSG( m_component, \
                                                                  m_device_object, \
                                                                  m_msg_string, \
                                                                  m_status_code ) \
        K10_COMMON_TRACE_DEVICE_WITH_FORMAT_PARAM( K10_COMMON_TRACE_IOCTL_FLARE_TRESPASS_OWNERSHIP_LOSS_STR, \
                                                   m_component, \
                                                   "", \
                                                   m_device_object, \
                                                   K10_COMMON_TRACE_UNUSED_PARAMETER, \
                                                   "\"" m_msg_string "\"" " 0x%x", \
                                                   m_status_code )

//
// Macro:       K10_COMMON_TRACE_IOCTL_FLARE_TRESPASS_OWNERSHIP_LOSS_MSG_BY_LUN
//
// Description: Used to display additional information on the operation.  
//
// Parameters:  m_component
//                  The identifier of the caller.
//
//              m_device_object
//                  The pointer to the device object.
//
//              m_lun
//                  The LUN that corresponds to this device.
//
//              m_msg_string
//                  A msg string to display to the user.
//
//              m_status_code
//                  A unique status code the corresponds to the msg string.
//                  Must be 0 if there is no appropriate status code.
//

#define K10_COMMON_TRACE_IOCTL_FLARE_TRESPASS_OWNERSHIP_LOSS_MSG_BY_LUN( m_component, \
                                                                         m_device_object, \
                                                                         m_lun, \
                                                                         m_msg_string, \
                                                                         m_status_code ) \
        K10_COMMON_TRACE_DEVICE_WITH_FORMAT_PARAM( K10_COMMON_TRACE_IOCTL_FLARE_TRESPASS_OWNERSHIP_LOSS_STR, \
                                                   m_component, \
                                                   "", \
                                                   m_device_object, \
                                                   m_lun, \
                                                   "\"" m_msg_string "\"" " 0x%x", \
                                                   m_status_code )

//*****************************************************************************
//*****************************************************************************
// IOCTL_FLARE_TRESPASS_EXECUTE
//*****************************************************************************
//*****************************************************************************


//
// Macro:       K10_COMMON_TRACE_IOCTL_FLARE_TRESPASS_EXECUTE_RECEPTION
//
// Description: Used upon operation reception.  If the LUN is known, use the
//              "BY_LUN" version of this macro.
//
// Parameters:  m_component
//                  The identifier of the caller.
//
//              m_device_name
//                  The string that represents the device name.
//
//              m_device_object
//                  The pointer to the device object.
//

#define K10_COMMON_TRACE_IOCTL_FLARE_TRESPASS_EXECUTE_RECEPTION( m_component, \
                                                                 m_device_name, \
                                                                 m_device_object ) \
        K10_COMMON_TRACE_DEVICE_WITH_FORMAT_PARAM( K10_COMMON_TRACE_IOCTL_FLARE_TRESPASS_EXECUTE_STR, \
                                                   m_component, \
                                                   m_device_name, \
                                                   m_device_object, \
                                                   K10_COMMON_TRACE_UNUSED_PARAMETER, \
                                                   "%s", \
                                                   "Received" )

//
// Macro:       K10_COMMON_TRACE_IOCTL_FLARE_TRESPASS_EXECUTE_RECEPTION_BY_LUN
//
// Description: Used upon operation reception.
//
// Parameters:  m_component
//                  The identifier of the caller.
//
//              m_device_name
//                  The string that represents the device name.
//
//              m_device_object
//                  The pointer to the device object.
//
//              m_lun
//                  The LUN that corresponds to this device.
//

#define K10_COMMON_TRACE_IOCTL_FLARE_TRESPASS_EXECUTE_RECEPTION_BY_LUN( m_component, \
                                                                        m_device_name, \
                                                                        m_device_object, \
                                                                        m_lun ) \
        K10_COMMON_TRACE_DEVICE_WITH_FORMAT_PARAM( K10_COMMON_TRACE_IOCTL_FLARE_TRESPASS_EXECUTE_STR, \
                                                   m_component, \
                                                   m_device_name, \
                                                   m_device_object, \
                                                   m_lun, \
                                                   "%s", \
                                                   "Received" )

//
// Macro:       K10_COMMON_TRACE_IOCTL_FLARE_TRESPASS_EXECUTE_COMPLETION
//
// Description: Used upon operation reception.  If the LUN is known, use the
//              "BY_LUN" version of this macro.
//
// Parameters:  m_component
//                  The identifier of the caller.
//
//              m_device_name
//                  The string that represents the device name.
//
//              m_device_object
//                  The pointer to the device object.
//

#define K10_COMMON_TRACE_IOCTL_FLARE_TRESPASS_EXECUTE_COMPLETION( m_component, \
                                                                  m_device_name, \
                                                                  m_device_object ) \
        K10_COMMON_TRACE_DEVICE_WITH_FORMAT_PARAM( K10_COMMON_TRACE_IOCTL_FLARE_TRESPASS_EXECUTE_STR, \
                                                   m_component, \
                                                   m_device_name, \
                                                   m_device_object, \
                                                   K10_COMMON_TRACE_UNUSED_PARAMETER, \
                                                   "%s", \
                                                   "Complete" )

//
// Macro:       K10_COMMON_TRACE_IOCTL_FLARE_TRESPASS_EXECUTE_COMPLETION_BY_LUN
//
// Description: Used upon operation reception.
//
// Parameters:  m_component
//                  The identifier of the caller.
//
//              m_device_name
//                  The string that represents the device name.
//
//              m_device_object
//                  The pointer to the device object.
//
//              m_lun
//                  The LUN that corresponds to this device.
//

#define K10_COMMON_TRACE_IOCTL_FLARE_TRESPASS_EXECUTE_COMPLETION_BY_LUN( m_component, \
                                                                         m_device_name, \
                                                                         m_device_object, \
                                                                         m_lun ) \
        K10_COMMON_TRACE_DEVICE_WITH_FORMAT_PARAM( K10_COMMON_TRACE_IOCTL_FLARE_TRESPASS_EXECUTE_STR, \
                                                   m_component, \
                                                   m_device_name, \
                                                   m_device_object, \
                                                   m_lun, \
                                                   "%s", \
                                                   "Complete" )

//
// Macro:       K10_COMMON_TRACE_IOCTL_FLARE_TRESPASS_EXECUTE_ACTUALLY_EXECUTING
//
// Description: Used when the device below is opened.  If the LUN is known,
//              use the "BY_LUN" version of this macro.
//
// Parameters:  m_component
//                  The identifier of the caller.
//
//              m_consumed_device_name
//                  The string that represents the consumed device name.
//
//              m_exported_device_object
//                  The pointer to the exported device object.
//

#define K10_COMMON_TRACE_IOCTL_FLARE_TRESPASS_EXECUTE_ACTUALLY_EXECUTING( m_component, \
                                                                          m_consumed_device_name, \
                                                                          m_exported_device_object ) \
        K10_COMMON_TRACE_DEVICE_WITH_FORMAT_PARAM( K10_COMMON_TRACE_IOCTL_FLARE_TRESPASS_EXECUTE_STR " Actually Executing", \
                                                   m_component, \
                                                   m_consumed_device_name, \
                                                   m_exported_device_object, \
                                                   K10_COMMON_TRACE_UNUSED_PARAMETER, \
                                                   "%s", \
                                                   "" )

//
// Macro:       K10_COMMON_TRACE_IOCTL_FLARE_TRESPASS_EXECUTE_ACTUALLY_EXECUTING_BY_LUN
//
// Description: Used when the device below is opened.
//
// Parameters:  m_component
//                  The identifier of the caller.
//
//              m_consumed_device_name
//                  The string that represents the consumed device name.
//
//              m_exported_device_object
//                  The pointer to the exported device object.
//
//              m_lun
//                  The LUN that corresponds to this device.
//

#define K10_COMMON_TRACE_IOCTL_FLARE_TRESPASS_EXECUTE_ACTUALLY_EXECUTING_BY_LUN( m_component, \
                                                                                 m_consumed_device_name, \
                                                                                 m_exported_device_object, \
                                                                                 m_lun ) \
        K10_COMMON_TRACE_DEVICE_WITH_FORMAT_PARAM( K10_COMMON_TRACE_IOCTL_FLARE_TRESPASS_EXECUTE_STR " Actually Executing", \
                                                    m_component, \
                                                    m_consumed_device_name, \
                                                    m_exported_device_object, \
                                                    m_lun, \
                                                    "%s", \
                                                    "" )


//
// Macro:       K10_COMMON_TRACE_IOCTL_FLARE_TRESPASS_EXECUTE_HOLDING
//
// Description: Used when the device below is opened.  If the LUN is known,
//              use the "BY_LUN" version of this macro.
//
// Parameters:  m_component
//                  The identifier of the caller.
//
//              m_consumed_device_name
//                  The string that represents the consumed device name.
//
//              m_exported_device_object
//                  The pointer to the exported device object.
//

#define K10_COMMON_TRACE_IOCTL_FLARE_TRESPASS_EXECUTE_HOLDING( m_component, \
                                                               m_consumed_device_name, \
                                                               m_exported_device_object ) \
        K10_COMMON_TRACE_DEVICE_WITH_FORMAT_PARAM( K10_COMMON_TRACE_IOCTL_FLARE_TRESPASS_EXECUTE_STR " Holding", \
                                                   m_component, \
                                                   m_consumed_device_name, \
                                                   m_exported_device_object, \
                                                   K10_COMMON_TRACE_UNUSED_PARAMETER, \
                                                   "%s", \
                                                   "" )

//
// Macro:       K10_COMMON_TRACE_IOCTL_FLARE_TRESPASS_EXECUTE_HOLDING_BY_LUN
//
// Description: Used when the device below is opened.
//
// Parameters:  m_component
//                  The identifier of the caller.
//
//              m_consumed_device_name
//                  The string that represents the consumed device name.
//
//              m_exported_device_object
//                  The pointer to the exported device object.
//
//              m_lun
//                  The LUN that corresponds to this device.
//

#define K10_COMMON_TRACE_IOCTL_FLARE_TRESPASS_EXECUTE_HOLDING_BY_LUN( m_component, \
                                                                      m_consumed_device_name, \
                                                                      m_exported_device_object, \
                                                                      m_lun ) \
        K10_COMMON_TRACE_DEVICE_WITH_FORMAT_PARAM( K10_COMMON_TRACE_IOCTL_FLARE_TRESPASS_EXECUTE_STR " Holding", \
                                                    m_component, \
                                                    m_consumed_device_name, \
                                                    m_exported_device_object, \
                                                    m_lun, \
                                                    "%s", \
                                                    "" )

//
// Macro:       K10_COMMON_TRACE_IOCTL_FLARE_TRESPASS_EXECUTE_MSG
//
// Description: Used to display additional information on the operation.  
//              If the LUN is known, use the "BY_LUN" version of this macro.
//
// Parameters:  m_component
//                  The identifier of the caller.
//
//              m_device_object
//                  The pointer to the device object.
//
//              m_msg_string
//                  A msg string to display to the user.
//
//              m_status_code
//                  A unique status code the corresponds to the msg string.
//                  Must be 0 if there is no appropriate status code.
//

#define K10_COMMON_TRACE_IOCTL_FLARE_TRESPASS_EXECUTE_MSG( m_component, \
                                                           m_device_object, \
                                                           m_msg_string, \
                                                           m_status_code ) \
        K10_COMMON_TRACE_DEVICE_WITH_FORMAT_PARAM( K10_COMMON_TRACE_IOCTL_FLARE_TRESPASS_EXECUTE_STR, \
                                                   m_component, \
                                                   "", \
                                                   m_device_object, \
                                                   K10_COMMON_TRACE_UNUSED_PARAMETER, \
                                                   "\"" m_msg_string "\"" " 0x%x", \
                                                   m_status_code )

//
// Macro:       K10_COMMON_TRACE_IOCTL_FLARE_TRESPASS_EXECUTE_MSG_BY_LUN
//
// Description: Used to display additional information on the operation.  
//
// Parameters:  m_component
//                  The identifier of the caller.
//
//              m_device_object
//                  The pointer to the device object.
//
//              m_lun
//                  The LUN that corresponds to this device.
//
//              m_msg_string
//                  A msg string to display to the user.
//
//              m_status_code
//                  A unique status code the corresponds to the msg string.
//                  Must be 0 if there is no appropriate status code.
//

#define K10_COMMON_TRACE_IOCTL_FLARE_TRESPASS_EXECUTE_MSG_BY_LUN( m_component, \
                                                                  m_device_object, \
                                                                  m_lun, \
                                                                  m_msg_string, \
                                                                  m_status_code ) \
        K10_COMMON_TRACE_DEVICE_WITH_FORMAT_PARAM( K10_COMMON_TRACE_IOCTL_FLARE_TRESPASS_EXECUTE_STR, \
                                                   m_component, \
                                                   "", \
                                                   m_device_object, \
                                                   m_lun, \
                                                   "\"" m_msg_string "\"" " 0x%x", \
                                                   m_status_code )

//*****************************************************************************
//*****************************************************************************
// IOCTL_FLARE_DCA_QUERY
//*****************************************************************************
//*****************************************************************************


//
// Macro:       K10_COMMON_TRACE_IOCTL_FLARE_DCA_QUERY_SUPPORT
//
// Description: Used to display additional information on the operation.  
//              If the LUN is known, use the "BY_LUN" version of this macro.
//
// Parameters:  m_component
//                  The identifier of the caller.
//
//              m_device_name
//                  The string that represents the consumed device name.
//
//              m_device_object
//                  The pointer to the device object.
//
//              m_dca_support
//                  The level of DCA support for this device.
//

#define K10_COMMON_TRACE_IOCTL_FLARE_DCA_QUERY_SUPPORT( m_component, \
                                                        m_device_name, \
                                                        m_device_object, \
                                                        m_dca_support ) \
        K10_COMMON_TRACE_DEVICE_WITH_FORMAT_PARAM( K10_COMMON_TRACE_IOCTL_FLARE_DCA_QUERY_STR, \
                                                   m_component, \
                                                   m_device_name, \
                                                   m_device_object, \
                                                   K10_COMMON_TRACE_UNUSED_PARAMETER, \
                                                   "%s", \
                                                   ( m_dca_support == FLARE_DCA_NEITHER_SUPPORTED ) ? "Neither" : \
                                                   ( m_dca_support == FLARE_DCA_READ_SUPPORTED )    ? "Read" : \
                                                   ( m_dca_support == FLARE_DCA_WRITE_SUPPORTED )   ? "Write" : \
                                                   ( m_dca_support == FLARE_DCA_BOTH_SUPPORTED )    ? "Both" : \
                                                   "Unknown!" )

//
// Macro:       K10_COMMON_TRACE_IOCTL_FLARE_DCA_QUERY_MSG
//
// Description: Used to display additional information on the operation.  
//              If the LUN is known, use the "BY_LUN" version of this macro.
//
// Parameters:  m_component
//                  The identifier of the caller.
//
//              m_device_object
//                  The pointer to the device object.
//
//              m_msg_string
//                  A msg string to display to the user.
//
//              m_status_code
//                  A unique status code the corresponds to the msg string.
//                  Must be 0 if there is no appropriate status code.
//

#define K10_COMMON_TRACE_IOCTL_FLARE_DCA_QUERY_MSG( m_component, \
                                                    m_device_object, \
                                                    m_msg_string, \
                                                    m_status_code ) \
        K10_COMMON_TRACE_DEVICE_WITH_FORMAT_PARAM( K10_COMMON_TRACE_IOCTL_FLARE_DCA_QUERY_STR, \
                                                   m_component, \
                                                   "", \
                                                   m_device_object, \
                                                   K10_COMMON_TRACE_UNUSED_PARAMETER, \
                                                   "\"" m_msg_string "\"" " 0x%x", \
                                                   m_status_code )

//
// Macro:       K10_COMMON_TRACE_IOCTL_FLARE_DCA_QUERY_MSG_BY_LUN
//
// Description: Used to display additional information on the operation.  
//
// Parameters:  m_component
//                  The identifier of the caller.
//
//              m_device_object
//                  The pointer to the device object.
//
//              m_lun
//                  The LUN that corresponds to this device.
//
//              m_msg_string
//                  A msg string to display to the user.
//
//              m_status_code
//                  A unique status code the corresponds to the msg string.
//                  Must be 0 if there is no appropriate status code.
//

#define K10_COMMON_TRACE_IOCTL_FLARE_DCA_QUERY_MSG_BY_LUN( m_component, \
                                                           m_device_object, \
                                                           m_lun, \
                                                           m_msg_string, \
                                                           m_status_code ) \
        K10_COMMON_TRACE_DEVICE_WITH_FORMAT_PARAM( K10_COMMON_TRACE_IOCTL_FLARE_DCA_QUERY_STR, \
                                                   m_component, \
                                                   "", \
                                                   m_device_object, \
                                                   m_lun, \
                                                   "\"" m_msg_string "\"" " 0x%x", \
                                                   m_status_code )


//*****************************************************************************
//*****************************************************************************
// IOCTL_K10ATTACH_BIND
//*****************************************************************************
//*****************************************************************************

//
// Macro:       K10_COMMON_TRACE_IOCTL_K10ATTACH_BIND_INPUT
//
// Description: Used upon operation reception.
//
// Parameters:  m_component
//                  The identifier of the caller.
//
//              m_consumed_device_name
//                  The string that represents the device name.
//
//              m_is_rebind
//                  A boolean indicating whether or not the request is a
//                  "Rebind" operation.
//

#define K10_COMMON_TRACE_IOCTL_K10ATTACH_BIND_INPUT( m_component, \
                                                     m_consumed_device_name, \
                                                     m_is_rebind ) \
        K10_COMMON_TRACE_DISPLAY_ROUTINE( \
            K10_COMMON_TRACE_IDENTIFICATION_STAMP \
            m_component \
            K10_COMMON_TRACE_IOCTL_K10ATTACH_BIND_STR \
            " Bind to %s %s\n", \
            m_consumed_device_name, m_is_rebind ? "Rebind" : "Not rebind" )


//
// Macro:       K10_COMMON_TRACE_IOCTL_K10ATTACH_BIND_OUTPUT
//
// Description: Used upon operation reception.
//
// Parameters:  m_component
//                  The identifier of the caller.
//
//              m_exported_device_name
//                  The string that represents the device name.
//
//              m_exported_device_object
//                  The pointer to the exported device object.
//

#define K10_COMMON_TRACE_IOCTL_K10ATTACH_BIND_OUTPUT( m_component, \
                                                      m_exported_device_name, \
                                                      m_exported_device_object ) \
        K10_COMMON_TRACE_DISPLAY_ROUTINE( \
            K10_COMMON_TRACE_IDENTIFICATION_STAMP \
            m_component \
            K10_COMMON_TRACE_IOCTL_K10ATTACH_BIND_STR \
            " Exporting %s 0x%x\n", \
            m_exported_device_name, m_exported_device_object )            

//
// Macro:       K10_COMMON_TRACE_IOCTL_K10ATTACH_BIND_MSG
//
// Description: Used to display additional information on the operation.
//
// Parameters:  m_component
//                  The identifier of the caller.
//
//              m_device_object
//                  The pointer to the device object.
//
//              m_msg_string
//                  A msg string to display to the user.
//
//              m_status_code
//                  A unique status code the corresponds to the msg string.
//                  Must be 0 if there is no appropriate status code.
//

#define K10_COMMON_TRACE_IOCTL_K10ATTACH_BIND_MSG( m_component, \
                                                   m_device_object, \
                                                   m_msg_string, \
                                                   m_status_code ) \
        K10_COMMON_TRACE_DEVICE_WITH_FORMAT_PARAM( K10_COMMON_TRACE_IOCTL_K10ATTACH_BIND_STR, \
                                                   m_component, \
                                                   "", \
                                                   m_device_object, \
                                                   K10_COMMON_TRACE_UNUSED_PARAMETER, \
                                                   "\"" m_msg_string "\"" " 0x%x", \
                                                   m_status_code )
            


//*****************************************************************************
//*****************************************************************************
// IOCTL_K10ATTACH_UNBIND
//*****************************************************************************
//*****************************************************************************

//
// Macro:       K10_COMMON_TRACE_IOCTL_K10ATTACH_UNBIND_INPUT
//
// Description: Used upon operation reception.
//
// Parameters:  m_component
//                  The identifier of the caller.
//
//              m_exported_device_name
//                  The string that represents the device name.
//
//              m_will_rebind
//                  A boolean indicating whether or not the request will be
//                  be followed by bind operation.
//

#define K10_COMMON_TRACE_IOCTL_K10ATTACH_UNBIND_INPUT( m_component, \
                                                       m_exported_device_name, \
                                                       m_is_rebind ) \
        K10_COMMON_TRACE_DISPLAY_ROUTINE( \
            K10_COMMON_TRACE_IDENTIFICATION_STAMP \
            m_component \
            K10_COMMON_TRACE_IOCTL_K10ATTACH_UNBIND_STR \
            " Unbind %s %s\n", \
            m_exported_device_name, m_is_rebind ? "Will rebind" : "Won't rebind" )


//
// Macro:       K10_COMMON_TRACE_IOCTL_K10ATTACH_UNBIND_DETAILS
//
// Description: Used upon operation reception.
//
// Parameters:  m_component
//                  The identifier of the caller.
//
//              m_consumed_device_name
//                  The string that represents the device name.
//

#define K10_COMMON_TRACE_IOCTL_K10ATTACH_UNBIND_DETAILS( m_component, \
                                                         m_consumed_device_name ) \
        K10_COMMON_TRACE_DISPLAY_ROUTINE( \
            K10_COMMON_TRACE_IDENTIFICATION_STAMP \
            m_component \
            K10_COMMON_TRACE_IOCTL_K10ATTACH_UNBIND_STR \
            " Unbinding from %s\n", \
            m_consumed_device_name )


//
// Macro:       K10_COMMON_TRACE_IOCTL_K10ATTACH_UNBIND_MSG
//
// Description: Used to display additional information on the operation.
//
// Parameters:  m_component
//                  The identifier of the caller.
//
//              m_device_object
//                  The pointer to the device object.
//
//              m_msg_string
//                  A msg string to display to the user.
//
//              m_status_code
//                  A unique status code the corresponds to the msg string.
//                  Must be 0 if there is no appropriate status code.
//

#define K10_COMMON_TRACE_IOCTL_K10ATTACH_UNBIND_MSG( m_component, \
                                                   m_device_object, \
                                                   m_msg_string, \
                                                   m_status_code ) \
        K10_COMMON_TRACE_DEVICE_WITH_FORMAT_PARAM( K10_COMMON_TRACE_IOCTL_K10ATTACH_UNBIND_STR, \
                                                   m_component, \
                                                   "", \
                                                   m_device_object, \
                                                   K10_COMMON_TRACE_UNUSED_PARAMETER, \
                                                   "\"" m_msg_string "\"" " 0x%x", \
                                                   m_status_code )


#endif // K10_COMMON_TRACE_H

