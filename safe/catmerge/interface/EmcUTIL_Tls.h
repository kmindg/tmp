#ifndef EMCUTIL_TLS_H_
#define EMCUTIL_TLS_H_

//***************************************************************************
// Copyright (C) EMC Corporation 2011
// All rights reserved.
// Licensed material -- property of EMC Corporation
//**************************************************************************/

/*!
 * @file EmcUTIL_Tls.h
 * @addtogroup emcutil_tls
 * @{
 * @brief
 *
 */

#include "EmcUTIL.h"

#if defined(UMODE_ENV) || defined(SIMMODE_ENV)

#ifndef ALAMOSA_WINDOWS_ENV
CSX_STATIC_INLINE DWORD
EmcutilTlsAlloc(VOID)
{
    csx_status_e Status;
    csx_p_thr_tsv_id_t CsxTsvId;
    Status = csx_p_thr_tsv_allocate(&CsxTsvId);
    CSX_ASSERT_SUCCESS_H_RDC(Status);
    return (DWORD) CsxTsvId;
}
CSX_STATIC_INLINE LPVOID
EmcutilTlsGetValue(DWORD dwTlsIndex)
{
    csx_status_e Status;
    csx_p_thr_tsv_id_t CsxTsvId = (csx_p_thr_tsv_id_t) dwTlsIndex;
    csx_p_thr_tsv_value_t CsxTsvValue;
    Status = csx_p_thr_tsv_get(CsxTsvId, &CsxTsvValue);
    CSX_ASSERT_SUCCESS_H_RDC(Status);
    return (LPVOID) CsxTsvValue;
}
CSX_STATIC_INLINE BOOL
EmcutilTlsSetValue(DWORD dwTlsIndex, LPVOID lpTlsValue)
{
    csx_status_e Status;
    csx_p_thr_tsv_id_t CsxTsvId = (csx_p_thr_tsv_id_t) dwTlsIndex;
    Status = csx_p_thr_tsv_set(CsxTsvId, (csx_p_thr_tsv_value_t) lpTlsValue);
    CSX_ASSERT_SUCCESS_H_RDC(Status);
    return TRUE;
}
CSX_STATIC_INLINE BOOL
EmcutilTlsFree(DWORD dwTlsIndex)
{
    csx_status_e Status;
    csx_p_thr_tsv_id_t CsxTsvId = (csx_p_thr_tsv_id_t) dwTlsIndex;
    Status = csx_p_thr_tsv_free(CsxTsvId);
    CSX_ASSERT_SUCCESS_H_RDC(Status);
    return TRUE;
}
CSX_STATIC_INLINE ULONG
EmcutilTlsGetLastError(VOID)
{
    return EMCUTIL_STATUS_OK;
}
#define EMCUTIL_TLS_OUT_OF_INDEXES ((DWORD)-1)
#else
#define EmcutilTlsAlloc TlsAlloc
#define EmcutilTlsGetValue TlsGetValue
#define EmcutilTlsSetValue TlsSetValue
#define EmcutilTlsFree TlsFree
#define EmcutilTlsGetLastError GetLastError
#define EMCUTIL_TLS_OUT_OF_INDEXES TLS_OUT_OF_INDEXES
#endif /* ALAMOSA_WINDOWS_ENV - STDPORT */

#endif


//++
//.End file EmcUTIL_Tls.h
//--

/*!
 * @} end group emcutil_tls
 * @} end file EmcUTIL_Tls.h
 */

#endif                                     /* EMCUTIL_TLS_H_ */
