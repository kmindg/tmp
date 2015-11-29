#ifdef ALAMOSA_WINDOWS_ENV
/* this ALWAYS GENERATED file contains the definitions for the interfaces */


/* File created by MIDL compiler version 3.01.75 */
/* at Tue Jun 22 09:07:47 1999
 */
/* Compiler settings for K10AdminExport.idl:
    Os (OptLev=s), W1, Zp8, env=Win32, ms_ext, c_ext
    error checks: none
*/
//@@MIDL_FILE_HEADING(  )
#include "stdafx_rpc.h"
#include "csx_ext.h"
#ifndef COM_NO_WINDOWS_H
#include "windows.h"
//#include "ole2.h"
#endif /*COM_NO_WINDOWS_H*/

#ifndef __K10AdminExport_h__
#define __K10AdminExport_h__

#ifdef __cplusplus
extern "C"{
#endif 

/* Forward Declarations */ 

#ifndef __IK10Admin_FWD_DEFINED__
#define __IK10Admin_FWD_DEFINED__
typedef interface IK10Admin IK10Admin;
#endif 	/* __IK10Admin_FWD_DEFINED__ */


#ifndef __K10Admin_FWD_DEFINED__
#define __K10Admin_FWD_DEFINED__

#ifdef __cplusplus
typedef class K10Admin K10Admin;
#else
typedef struct K10Admin K10Admin;
#endif /* __cplusplus */

#endif 	/* __K10Admin_FWD_DEFINED__ */


/* header files for imported files */
//#include "oaidl.h"

void __RPC_FAR * __RPC_USER MIDL_user_allocate(size_t);
void __RPC_USER MIDL_user_free( void __RPC_FAR * ); 

/****************************************
 * Generated header for interface: __MIDL_itf_K10AdminExport_0000
 * at Tue Jun 22 09:07:47 1999
 * using MIDL 3.01.75
 ****************************************/
/* [local] */ 


typedef unsigned char __RPC_FAR *TLD_POINTER;



extern RPC_IF_HANDLE __MIDL_itf_K10AdminExport_0000_v0_0_c_ifspec;
extern RPC_IF_HANDLE __MIDL_itf_K10AdminExport_0000_v0_0_s_ifspec;

#ifndef __IK10Admin_INTERFACE_DEFINED__
#define __IK10Admin_INTERFACE_DEFINED__

/****************************************
 * Generated header for interface: IK10Admin
 * at Tue Jun 22 09:07:47 1999
 * using MIDL 3.01.75
 ****************************************/
/* [unique][helpstring][uuid][object] */ 



EXTERN_C const IID IID_IK10Admin;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    interface DECLSPEC_UUID("1d0eb802-771b-11d1-8054-0060b01a1c6e")
    IK10Admin : public IDispatch
    {
    public:
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE Write( 
            /* [in] */ long lDbField,
            /* [in] */ long lItemSpec,
            /* [in] */ long lOpCode,
            /* [in] */ csx_uchar_t __RPC_FAR *pIn,
            /* [in] */ long lBufSize,
            /* [retval][out] */ long __RPC_FAR *lError) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE Read( 
            /* [in] */ long lDbField,
            /* [in] */ long lItemSpec,
            /* [in] */ long lOpCode,
            /* [out][in] */ csx_uchar_t __RPC_FAR *__RPC_FAR *ppOut,
            /* [out][in] */ long __RPC_FAR *lpBufSize,
            /* [retval][out] */ long __RPC_FAR *lError) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE Update( 
            /* [in] */ long lDbField,
            /* [in] */ long lItemSpec,
            /* [in] */ long lOpCode,
            /* [in] */ csx_uchar_t __RPC_FAR *pInOut,
            /* [in] */ long lBufSize,
            /* [retval][out] */ long __RPC_FAR *lError) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE ReadTldList( 
            /* [in] */ TLD_POINTER pTldParamList,
            /* [out] */ TLD_POINTER __RPC_FAR *ppTldResultList,
            /* [retval][out] */ long __RPC_FAR *lError) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE WriteTldList( 
            /* [in] */ TLD_POINTER pTldList,
            /* [retval][out] */ long __RPC_FAR *lError) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IK10AdminVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            IK10Admin __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            IK10Admin __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            IK10Admin __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetTypeInfoCount )( 
            IK10Admin __RPC_FAR * This,
            /* [out] */ UINT __RPC_FAR *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetTypeInfo )( 
            IK10Admin __RPC_FAR * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo __RPC_FAR *__RPC_FAR *ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetIDsOfNames )( 
            IK10Admin __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR __RPC_FAR *rgszNames,
            /* [in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID __RPC_FAR *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Invoke )( 
            IK10Admin __RPC_FAR * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS __RPC_FAR *pDispParams,
            /* [out] */ VARIANT __RPC_FAR *pVarResult,
            /* [out] */ EXCEPINFO __RPC_FAR *pExcepInfo,
            /* [out] */ UINT __RPC_FAR *puArgErr);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Write )( 
            IK10Admin __RPC_FAR * This,
            /* [in] */ long lDbField,
            /* [in] */ long lItemSpec,
            /* [in] */ long lOpCode,
            /* [in] */ csx_uchar_t __RPC_FAR *pIn,
            /* [in] */ long lBufSize,
            /* [retval][out] */ long __RPC_FAR *lError);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Read )( 
            IK10Admin __RPC_FAR * This,
            /* [in] */ long lDbField,
            /* [in] */ long lItemSpec,
            /* [in] */ long lOpCode,
            /* [out][in] */ csx_uchar_t __RPC_FAR *__RPC_FAR *ppOut,
            /* [out][in] */ long __RPC_FAR *lpBufSize,
            /* [retval][out] */ long __RPC_FAR *lError);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Update )( 
            IK10Admin __RPC_FAR * This,
            /* [in] */ long lDbField,
            /* [in] */ long lItemSpec,
            /* [in] */ long lOpCode,
            /* [in] */ csx_uchar_t __RPC_FAR *pInOut,
            /* [in] */ long lBufSize,
            /* [retval][out] */ long __RPC_FAR *lError);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *ReadTldList )( 
            IK10Admin __RPC_FAR * This,
            /* [in] */ TLD_POINTER pTldParamList,
            /* [out] */ TLD_POINTER __RPC_FAR *ppTldResultList,
            /* [retval][out] */ long __RPC_FAR *lError);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *WriteTldList )( 
            IK10Admin __RPC_FAR * This,
            /* [in] */ TLD_POINTER pTldList,
            /* [retval][out] */ long __RPC_FAR *lError);
        
        END_INTERFACE
    } IK10AdminVtbl;

    interface IK10Admin
    {
        CONST_VTBL struct IK10AdminVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IK10Admin_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IK10Admin_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IK10Admin_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IK10Admin_GetTypeInfoCount(This,pctinfo)	\
    (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo)

#define IK10Admin_GetTypeInfo(This,iTInfo,lcid,ppTInfo)	\
    (This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo)

#define IK10Admin_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)	\
    (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)

#define IK10Admin_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)	\
    (This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)


#define IK10Admin_Write(This,lDbField,lItemSpec,lOpCode,pIn,lBufSize,lError)	\
    (This)->lpVtbl -> Write(This,lDbField,lItemSpec,lOpCode,pIn,lBufSize,lError)

#define IK10Admin_Read(This,lDbField,lItemSpec,lOpCode,ppOut,lpBufSize,lError)	\
    (This)->lpVtbl -> Read(This,lDbField,lItemSpec,lOpCode,ppOut,lpBufSize,lError)

#define IK10Admin_Update(This,lDbField,lItemSpec,lOpCode,pInOut,lBufSize,lError)	\
    (This)->lpVtbl -> Update(This,lDbField,lItemSpec,lOpCode,pInOut,lBufSize,lError)

#define IK10Admin_ReadTldList(This,pTldParamList,ppTldResultList,lError)	\
    (This)->lpVtbl -> ReadTldList(This,pTldParamList,ppTldResultList,lError)

#define IK10Admin_WriteTldList(This,pTldList,lError)	\
    (This)->lpVtbl -> WriteTldList(This,pTldList,lError)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE IK10Admin_Write_Proxy( 
    IK10Admin __RPC_FAR * This,
    /* [in] */ long lDbField,
    /* [in] */ long lItemSpec,
    /* [in] */ long lOpCode,
    /* [in] */ csx_uchar_t __RPC_FAR *pIn,
    /* [in] */ long lBufSize,
    /* [retval][out] */ long __RPC_FAR *lError);


void __RPC_STUB IK10Admin_Write_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE IK10Admin_Read_Proxy( 
    IK10Admin __RPC_FAR * This,
    /* [in] */ long lDbField,
    /* [in] */ long lItemSpec,
    /* [in] */ long lOpCode,
    /* [out][in] */ csx_uchar_t __RPC_FAR *__RPC_FAR *ppOut,
    /* [out][in] */ long __RPC_FAR *lpBufSize,
    /* [retval][out] */ long __RPC_FAR *lError);


void __RPC_STUB IK10Admin_Read_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE IK10Admin_Update_Proxy( 
    IK10Admin __RPC_FAR * This,
    /* [in] */ long lDbField,
    /* [in] */ long lItemSpec,
    /* [in] */ long lOpCode,
    /* [in] */ csx_uchar_t __RPC_FAR *pInOut,
    /* [in] */ long lBufSize,
    /* [retval][out] */ long __RPC_FAR *lError);


void __RPC_STUB IK10Admin_Update_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE IK10Admin_ReadTldList_Proxy( 
    IK10Admin __RPC_FAR * This,
    /* [in] */ TLD_POINTER pTldParamList,
    /* [out] */ TLD_POINTER __RPC_FAR *ppTldResultList,
    /* [retval][out] */ long __RPC_FAR *lError);


void __RPC_STUB IK10Admin_ReadTldList_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE IK10Admin_WriteTldList_Proxy( 
    IK10Admin __RPC_FAR * This,
    /* [in] */ TLD_POINTER pTldList,
    /* [retval][out] */ long __RPC_FAR *lError);


void __RPC_STUB IK10Admin_WriteTldList_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IK10Admin_INTERFACE_DEFINED__ */



#ifndef __K10ADMINLib_LIBRARY_DEFINED__
#define __K10ADMINLib_LIBRARY_DEFINED__

/****************************************
 * Generated header for library: K10ADMINLib
 * at Tue Jun 22 09:07:47 1999
 * using MIDL 3.01.75
 ****************************************/
/* [helpstring][version][uuid] */ 



EXTERN_C const IID LIBID_K10ADMINLib;

#ifdef __cplusplus
EXTERN_C const CLSID CLSID_K10Admin;

class DECLSPEC_UUID("45112e52-771b-11d1-8054-0060b01a1c6e")
K10Admin;
#endif
#endif /* __K10ADMINLib_LIBRARY_DEFINED__ */

/* Additional Prototypes for ALL interfaces */

/* end of Additional Prototypes */

#ifdef __cplusplus
}
#endif

#endif
#endif /* ALAMOSA_WINDOWS_ENV - STDPORT - no need for this MFC/ATL junk off Windows */
