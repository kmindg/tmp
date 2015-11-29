/*!
 * @file EmcPAL_DriverShell_Extensions.h
 * @addtogroup emcpal_configuration
 * @{
 */
#ifndef EMCPAL_DRIVER_SHELL_EXTENSIONS_H
#define EMCPAL_DRIVER_SHELL_EXTENSIONS_H

//=======================
// EMCPAL Driver Object Manipulation

/*! @brief major functions count */
#ifdef EMCPAL_USE_CSX_DCALL
#define EMCPAL_NUM_MAJOR_FUNCTIONS CSX_P_DCALL_TYPE_CODE_COUNT
#else
#define EMCPAL_NUM_MAJOR_FUNCTIONS (IRP_MJ_MAXIMUM_FUNCTION+1)
#endif

//========

/*! @brief EmcpalExtDeviceAliasCreate - Create device alias
 *  @param SymbolicLinkName Symbolic link name
 *  @param DeviceName device name
 *  @return STATUS_SUCCESS if success, appropriate error-code otherwise
 */
CSX_STATIC_INLINE EMCPAL_STATUS
EmcpalExtDeviceAliasCreateW(
    EMCPAL_PUNICODE_STRING SymbolicLinkName,
    EMCPAL_PUNICODE_STRING DeviceName)
{
#ifdef EMCPAL_USE_CSX_DCALL
    csx_string_t DeviceNameNarrow;
    csx_string_t SymbolicLinkNameNarrow;
    csx_status_e Status;
    Status = csx_rt_mx_nt_RtlUnicodeStringToAsciiString(DeviceName, &DeviceNameNarrow);
    CSX_ASSERT_SUCCESS_H_RDC(Status);
    Status = csx_rt_mx_nt_RtlUnicodeStringToAsciiString(SymbolicLinkName, &SymbolicLinkNameNarrow);
    CSX_ASSERT_SUCCESS_H_RDC(Status);
    Status = csx_p_device_link_create(DeviceNameNarrow, SymbolicLinkNameNarrow);
    csx_rt_mx_nt_RtlFreeAsciiString(SymbolicLinkNameNarrow);
    csx_rt_mx_nt_RtlFreeAsciiString(DeviceNameNarrow);
    return Status;
#else                                      /* EMCPAL_USE_CSX_DCALL */
    return IoCreateSymbolicLink(SymbolicLinkName, DeviceName);
#endif                                     /* EMCPAL_USE_CSX_DCALL */
}

/********************************************************************
*! @brief EmcpalExtDeviceAliasCreateA - Create device alias
 *  @param SymbolicLinkName Symbolic link name
 *  @param DeviceName device name
 *  @return STATUS_SUCCESS if success, appropriate error-code otherwise
 */
CSX_STATIC_INLINE EMCPAL_STATUS
EmcpalExtDeviceAliasCreateA(
    csx_cstring_t SymbolicLinkName,
    csx_cstring_t DeviceName)
{
    EMCPAL_STATUS Status;
#ifdef EMCPAL_USE_CSX_DCALL
    Status = csx_p_device_link_create(DeviceName, SymbolicLinkName);

    return Status;
#else                                      /* EMCPAL_USE_CSX_DCALL */
    EMCPAL_UNICODE_STRING UnicodeSymbolicLinkName;
    EMCPAL_UNICODE_STRING UnicodeDeviceName;
    EMCPAL_ANSI_STRING    AnsiSymbolicLinkName;
    EMCPAL_ANSI_STRING    AnsiDeviceName;

    EmcpalInitAnsiString(&AnsiSymbolicLinkName, SymbolicLinkName);
    EmcpalInitAnsiString( &AnsiDeviceName, DeviceName);

    Status = EmcpalAnsiStringToUnicodeString( &UnicodeSymbolicLinkName, &AnsiSymbolicLinkName ,TRUE);

    if(CSX_FAILURE(Status))
    {
        goto bad;
    }
    
    Status = EmcpalAnsiStringToUnicodeString( &UnicodeDeviceName, &AnsiDeviceName ,TRUE);

    if(CSX_FAILURE(Status))
    {
        goto bad;
    }
        
    Status = (EMCPAL_STATUS) IoCreateSymbolicLink(&UnicodeSymbolicLinkName, &UnicodeDeviceName);

  bad:
    if(CSX_NOT_NULL(UnicodeSymbolicLinkName.Buffer))
    {
        EmcpalFreeUnicodeString( &UnicodeSymbolicLinkName);
    }
    if(CSX_NOT_NULL(UnicodeDeviceName.Buffer))
    {
        EmcpalFreeUnicodeString( &UnicodeDeviceName);
    }

    return Status;
#endif                                     /* EMCPAL_USE_CSX_DCALL */
}
/********************************************************************/
//========

/*! @brief EmcpalExtDeviceAliasDestroy - Destroy device alias
 *  @param SymbolicLinkName Symbolic link name
 *  @return STATUS_SUCCESS if success, appropriate error-code otherwise
 */
CSX_STATIC_INLINE EMCPAL_STATUS
EmcpalExtDeviceAliasDestroyW(
    EMCPAL_PUNICODE_STRING SymbolicLinkName)
{
#ifdef EMCPAL_USE_CSX_DCALL
    csx_string_t SymbolicLinkNameNarrow;
    csx_status_e Status;
    Status = csx_rt_mx_nt_RtlUnicodeStringToAsciiString(SymbolicLinkName, &SymbolicLinkNameNarrow);
    CSX_ASSERT_SUCCESS_H_RDC(Status);
    Status = csx_p_device_link_destroy(SymbolicLinkNameNarrow);
    csx_rt_mx_nt_RtlFreeAsciiString(SymbolicLinkNameNarrow);
    return Status;
#else                                      /* EMCPAL_USE_CSX_DCALL */
    return IoDeleteSymbolicLink(SymbolicLinkName);
#endif                                     /* EMCPAL_USE_CSX_DCALL */
}

/********************************************************************
 *! @brief EmcpalExtDeviceAliasDestroyA - Destroy device alias
 *  @param SymbolicLinkName Symbolic link name
 *  @return STATUS_SUCCESS if success, appropriate error-code otherwise
 */
CSX_STATIC_INLINE EMCPAL_STATUS
EmcpalExtDeviceAliasDestroyA(
    csx_cstring_t SymbolicLinkName)
{
    EMCPAL_STATUS Status;
#ifdef EMCPAL_USE_CSX_DCALL
    Status = csx_p_device_link_destroy(SymbolicLinkName);

    return Status;
#else                                      /* EMCPAL_USE_CSX_DCALL */
    EMCPAL_UNICODE_STRING   UnicodeSymbolicLinkName;
    EMCPAL_ANSI_STRING      AnsiSymbolicLinkName;

    EmcpalInitAnsiString( &AnsiSymbolicLinkName, SymbolicLinkName);
    Status = EmcpalAnsiStringToUnicodeString(&UnicodeSymbolicLinkName , &AnsiSymbolicLinkName , TRUE);

    if(CSX_SUCCESS(Status))
    {
        Status = (EMCPAL_STATUS) IoDeleteSymbolicLink(&UnicodeSymbolicLinkName);
    }

    if(CSX_NOT_NULL(UnicodeSymbolicLinkName.Buffer))
    {
        EmcpalFreeUnicodeString(&UnicodeSymbolicLinkName );
    }
    return Status;
#endif                                     /* EMCPAL_USE_CSX_DCALL */
}

/********************************************************************/
//========

#ifdef EMCPAL_USE_CSX_DCALL

#define EmcpalExtDeviceCreate(\
    _DriverObject, \
    _DeviceExtensionSize, \
    _DeviceName, \
    _DeviceType, \
    _DeviceCharacteristics, \
    _Exclusive, \
    _DeviceObjectRv) \
    EmcpalExtDeviceCreateImplA( \
        EmcpalDriverGetCurrentClientObject(), \
        _DriverObject, \
        _DeviceExtensionSize, \
        _DeviceName, \
        _DeviceType, \
        _DeviceCharacteristics, \
        _Exclusive, \
        _DeviceObjectRv)

#define EmcpalExtDeviceCreateW(\
    _DriverObject, \
    _DeviceExtensionSize, \
    _DeviceName, \
    _DeviceType, \
    _DeviceCharacteristics, \
    _Exclusive, \
    _DeviceObjectRv) \
    EmcpalExtDeviceCreateImplW( \
        EmcpalDriverGetCurrentClientObject(), \
        _DriverObject, \
        _DeviceExtensionSize, \
        _DeviceName, \
        _DeviceType, \
        _DeviceCharacteristics, \
        _Exclusive, \
        _DeviceObjectRv)

#else

#define EmcpalExtDeviceCreate(\
    _DriverObject, \
    _DeviceExtensionSize, \
    _DeviceName, \
    _DeviceType, \
    _DeviceCharacteristics, \
    _Exclusive, \
    _DeviceObjectRv) \
    EmcpalExtDeviceCreateImplA( \
        CSX_NULL, \
        _DriverObject, \
        _DeviceExtensionSize, \
        _DeviceName, \
        _DeviceType, \
        _DeviceCharacteristics, \
        _Exclusive, \
        _DeviceObjectRv)

#define EmcpalExtDeviceCreateW(\
    _DriverObject, \
    _DeviceExtensionSize, \
    _DeviceName, \
    _DeviceType, \
    _DeviceCharacteristics, \
    _Exclusive, \
    _DeviceObjectRv) \
    EmcpalExtDeviceCreateImplW( \
        CSX_NULL, \
        _DriverObject, \
        _DeviceExtensionSize, \
        _DeviceName, \
        _DeviceType, \
        _DeviceCharacteristics, \
        _Exclusive, \
        _DeviceObjectRv)


#endif

#ifdef SIMMODE_ENV
#define EmcpalExtDeviceCreate2(\
    _DriverObject, \
    _DeviceExtensionSize, \
    _DeviceName, \
    _DeviceType, \
    _DeviceCharacteristics, \
    _Exclusive, \
    _DeviceObjectRv) \
    EmcpalExtDeviceCreateImplA( \
        EmcpalDriverGetCurrentClientObject(), \
        _DriverObject, \
        _DeviceExtensionSize, \
        _DeviceName, \
        _DeviceType, \
        _DeviceCharacteristics, \
        _Exclusive, \
        _DeviceObjectRv)
#else
#define EmcpalExtDeviceCreate2(\
    _DriverObject, \
    _DeviceExtensionSize, \
    _DeviceName, \
    _DeviceType, \
    _DeviceCharacteristics, \
    _Exclusive, \
    _DeviceObjectRv) \
    EmcpalExtDeviceCreateImplA( \
        EmcPALDllGetClient(), \
        _DriverObject, \
        _DeviceExtensionSize, \
        _DeviceName, \
        _DeviceType, \
        _DeviceCharacteristics, \
        _Exclusive, \
        _DeviceObjectRv)
#endif

CSX_STATIC_INLINE EMCPAL_STATUS
EmcpalExtDeviceCreateImplW(
    PEMCPAL_CLIENT pPalClient,
    PEMCPAL_NATIVE_DRIVER_OBJECT DriverObject,
    ULONG DeviceExtensionSize,
    EMCPAL_PUNICODE_STRING DeviceName,
    ULONG DeviceType,
    ULONG DeviceCharacteristics,
    BOOLEAN Exclusive,
    PEMCPAL_DEVICE_OBJECT * DeviceObjectRv)
{
#ifdef EMCPAL_USE_CSX_DCALL
    EMCPAL_STATUS Status;
    csx_p_device_attributes_t CsxDeviceAttributes;
    csx_string_t CsxDeviceName = CSX_NULL;
    PEMCPAL_DEVICE_OBJECT CsxDeviceHandle;

//PZPZ revisit - this is foul - certain SIMMODE device creation code does not enable its created devices!
#ifndef SIMMODE_ENV
    CsxDeviceAttributes = CSX_P_DEVICE_ATTRIBUTE_START_DISABLED;
#else
    CsxDeviceAttributes = 0;
#endif
    if (CSX_IS_TRUE(Exclusive)) {
        CsxDeviceAttributes |= CSX_P_DEVICE_ATTRIBUTE_EXCLUSIVE;
    }

    if (CSX_NOT_NULL(DeviceName)) {
        Status = csx_rt_mx_nt_RtlUnicodeStringToAsciiString(DeviceName, &CsxDeviceName);
        if (CSX_FAILURE(Status)) {
            goto bad;
        }
    }

    if (CSX_NOT_NULL(DeviceName)) {
        Status = csx_p_device_create_with_name_with_attributes(EmcpalClientGetCsxModuleContext(pPalClient),
            DriverObject, CsxDeviceName, DeviceExtensionSize,
            CSX_NULL, CSX_ZERO_SIZE, CsxDeviceAttributes, &CsxDeviceHandle);
        if (CSX_FAILURE(Status)) {
            goto bad;
        }
    } else {
        Status = csx_p_device_create_unnamed_with_attributes(EmcpalClientGetCsxModuleContext(pPalClient),
            DriverObject, DeviceExtensionSize, CSX_NULL, CSX_ZERO_SIZE, CsxDeviceAttributes, &CsxDeviceHandle);
        if (CSX_FAILURE(Status)) {
            goto bad;
        }
    }
    csx_p_device_type_set(CsxDeviceHandle, DeviceType);
    csx_p_device_dcall_callbacks_set(CsxDeviceHandle, EmcpalDeviceIrpCallWrapper, EmcpalDeviceIrpCompletionWrapper);
    *DeviceObjectRv = CsxDeviceHandle;
    Status = CSX_STATUS_SUCCESS;
  bad:
    if (CSX_NOT_NULL(CsxDeviceName)) {
        csx_rt_mx_nt_RtlFreeAsciiString(CsxDeviceName);
    }
    return Status;
#else                                      /* EMCPAL_USE_CSX_DCALL */
    return IoCreateDevice(DriverObject, DeviceExtensionSize, DeviceName, DeviceType, DeviceCharacteristics, Exclusive, DeviceObjectRv);
#endif                                     /* EMCPAL_USE_CSX_DCALL */
}

CSX_STATIC_INLINE EMCPAL_STATUS
EmcpalExtDeviceCreateImplA(
    PEMCPAL_CLIENT pPalClient,
    PEMCPAL_NATIVE_DRIVER_OBJECT DriverObject,
    ULONG DeviceExtensionSize,
    csx_cstring_t DeviceName,
    ULONG DeviceType,
    ULONG DeviceCharacteristics,
    BOOLEAN Exclusive,
    PEMCPAL_DEVICE_OBJECT * DeviceObjectRv)
{
    EMCPAL_STATUS Status;
#ifdef EMCPAL_USE_CSX_DCALL
    csx_p_device_attributes_t CsxDeviceAttributes;
    PEMCPAL_DEVICE_OBJECT CsxDeviceHandle;

//PZPZ revisit - this is foul - certain SIMMODE device creation code does not enable its created devices!
#ifndef SIMMODE_ENV
    CsxDeviceAttributes = CSX_P_DEVICE_ATTRIBUTE_START_DISABLED;
#else
    CsxDeviceAttributes = 0;
#endif
    if (CSX_IS_TRUE(Exclusive)) {
        CsxDeviceAttributes |= CSX_P_DEVICE_ATTRIBUTE_EXCLUSIVE;
    }

    if (CSX_NOT_NULL(DeviceName)) {
        Status = csx_p_device_create_with_name_with_attributes(EmcpalClientGetCsxModuleContext(pPalClient),
            DriverObject, DeviceName, DeviceExtensionSize,
            CSX_NULL, CSX_ZERO_SIZE, CsxDeviceAttributes, &CsxDeviceHandle);
        if (CSX_FAILURE(Status)) {
            goto bad;
        }
    } else {
        Status = csx_p_device_create_unnamed_with_attributes(EmcpalClientGetCsxModuleContext(pPalClient),
            DriverObject, DeviceExtensionSize, CSX_NULL, CSX_ZERO_SIZE, CsxDeviceAttributes, &CsxDeviceHandle);
        if (CSX_FAILURE(Status)) {
            goto bad;
        }
    }
    csx_p_device_type_set(CsxDeviceHandle, DeviceType);
    csx_p_device_dcall_callbacks_set(CsxDeviceHandle, EmcpalDeviceIrpCallWrapper, EmcpalDeviceIrpCompletionWrapper);
    *DeviceObjectRv = CsxDeviceHandle;
    Status = CSX_STATUS_SUCCESS;
  bad:
    return Status;
#else                                      /* EMCPAL_USE_CSX_DCALL */
    EMCPAL_ANSI_STRING      AnsiDeviceName;
    EMCPAL_UNICODE_STRING   UnicodeDeviceName;

    EmcpalInitAnsiString(&AnsiDeviceName, DeviceName);
    Status = EmcpalAnsiStringToUnicodeString(&UnicodeDeviceName, &AnsiDeviceName, TRUE);

    if(CSX_FAILURE(Status))
    {
        goto bad;
    }
    
    Status = IoCreateDevice(DriverObject, DeviceExtensionSize, &UnicodeDeviceName, DeviceType, DeviceCharacteristics, Exclusive, DeviceObjectRv);

  bad:
    if(CSX_NOT_NULL(UnicodeDeviceName.Buffer))
    {
        EmcpalFreeUnicodeString(&UnicodeDeviceName);
    }

    return Status;
#endif                                     /* EMCPAL_USE_CSX_DCALL */
}

/*! @brief EmcpalExtDeviceDestroy - Destroy the device
 *  @param DeviceObject Device object
 *  @return none
 */
CSX_STATIC_INLINE VOID
EmcpalExtDeviceDestroy(
    PEMCPAL_DEVICE_OBJECT DeviceObject)
{
#ifdef EMCPAL_USE_CSX_DCALL
    csx_status_e Status;
    Status = csx_p_device_force_destroy(DeviceObject);
    CSX_ASSERT_SUCCESS_H_RDC(Status);
#else                                      /* EMCPAL_USE_CSX_DCALL */
    IoDeleteDevice(DeviceObject);
#endif                                     /* EMCPAL_USE_CSX_DCALL */
}

//========

/*! @brief EmcPAL raw IRP handler */
#ifdef EMCPAL_USE_CSX_DCALL
/* this is - and must remain - compatible with csx_p_dcall_handler_function_t */
typedef EMCPAL_STATUS (*EMCPAL_RAW_IRP_HANDLER) (PEMCPAL_DEVICE_OBJECT DeviceObject, PEMCPAL_IRP Irp);
#else                                      /* EMCPAL_USE_CSX_DCALL */
typedef PDRIVER_DISPATCH EMCPAL_RAW_IRP_HANDLER;
#endif                                     /* EMCPAL_USE_CSX_DCALL */

/*! @brief EmcpalExtDriverIrpHandlerSet - Set driver's IRP handler
 *  @param DriverObject Driver object
 *  @param IrpType IRP type
 *  @param IrpHandler IRP handler
 *  @return none
 */
CSX_STATIC_INLINE VOID
EmcpalExtDriverIrpHandlerSet(
    PEMCPAL_NATIVE_DRIVER_OBJECT DriverObject,
    ULONG IrpType,
    EMCPAL_RAW_IRP_HANDLER IrpHandler)
{
#ifdef EMCPAL_USE_CSX_DCALL
    csx_p_driver_handler_set(DriverObject, IrpType, (csx_p_dcall_handler_function_t)IrpHandler);
#else                                      /* EMCPAL_USE_CSX_DCALL */
    DriverObject->MajorFunction[IrpType] = IrpHandler;
#endif                                     /* EMCPAL_USE_CSX_DCALL */
}

//========

/* *INDENT-OFF* */
/*! @brief Raw start IO handler */
#ifdef EMCPAL_USE_CSX_DCALL
typedef VOID (*EMCPAL_RAW_START_IO_HANDLER) (PEMCPAL_DEVICE_OBJECT DeviceObject, PEMCPAL_IRP Irp);
#else                                      /* EMCPAL_USE_CSX_DCALL */
typedef PDRIVER_STARTIO EMCPAL_RAW_START_IO_HANDLER;
#endif                                     /* EMCPAL_USE_CSX_DCALL */
/* *INDENT-ON* */

/*! @brief EmcpalExtDriverStartIoHandlerSet - Set driver's start IO handler
 *  @param DriverObject Driver object
 *  @param StartIoHandler Start IO handler
 *  @return none
 */
CSX_STATIC_INLINE VOID
EmcpalExtDriverStartIoHandlerSet(
    PEMCPAL_NATIVE_DRIVER_OBJECT DriverObject,
    EMCPAL_RAW_START_IO_HANDLER StartIoHandler)
{
#ifdef EMCPAL_USE_CSX_DCALL
    csx_pvoid_t *driver_user_cookies_array = csx_p_driver_user_cookies_array_get(DriverObject);
    driver_user_cookies_array[0] = (csx_pvoid_t) StartIoHandler;
#else                                      /* EMCPAL_USE_CSX_DCALL */
    DriverObject->DriverStartIo = (PDRIVER_STARTIO) StartIoHandler;
#endif                                     /* EMCPAL_USE_CSX_DCALL */
}

//========

/* *INDENT-OFF* */
/*! @brief Add raw device handler */
#ifdef EMCPAL_USE_CSX_DCALL
typedef EMCPAL_STATUS (*EMCPAL_RAW_ADD_DEVICE_HANDLER) (PEMCPAL_NATIVE_DRIVER_OBJECT DriverObject, PEMCPAL_DEVICE_OBJECT DeviceObject);
#else                                      /* EMCPAL_USE_CSX_DCALL */
typedef PDRIVER_ADD_DEVICE EMCPAL_RAW_ADD_DEVICE_HANDLER;
#endif                                     /* EMCPAL_USE_CSX_DCALL */
/* *INDENT-ON* */

/*! @brief EmcpalExtDriverAddDeviceHandlerSet - Set the add device handler
 *  @param DriverObject Driver object
 *  @param AddDeviceHandler Add device handler
 *  @return none
 */
CSX_STATIC_INLINE VOID
EmcpalExtDriverAddDeviceHandlerSet(
    PEMCPAL_NATIVE_DRIVER_OBJECT DriverObject,
    EMCPAL_RAW_ADD_DEVICE_HANDLER AddDeviceHandler)
{
#ifdef EMCPAL_USE_CSX_DCALL
    csx_pvoid_t *driver_user_cookies_array = csx_p_driver_user_cookies_array_get(DriverObject);
    driver_user_cookies_array[1] = (csx_pvoid_t) AddDeviceHandler;
#else                                      /* EMCPAL_USE_CSX_DCALL */
    DriverObject->DriverExtension->AddDevice = AddDeviceHandler;
#endif                                     /* EMCPAL_USE_CSX_DCALL */
}

/*! @brief EmcpalExtDriverAddDeviceHandlerGet - Get the add device handler
 *  @param DriverObject Driver object
 *  @return 
 */
CSX_STATIC_INLINE EMCPAL_RAW_ADD_DEVICE_HANDLER
EmcpalExtDriverAddDeviceHandlerGet(
    PEMCPAL_NATIVE_DRIVER_OBJECT DriverObject)
{
#ifdef EMCPAL_USE_CSX_DCALL
    csx_pvoid_t *driver_user_cookies_array = csx_p_driver_user_cookies_array_get(DriverObject);
    return (EMCPAL_RAW_ADD_DEVICE_HANDLER) driver_user_cookies_array[1];
#else                                      /* EMCPAL_USE_CSX_DCALL */
    return DriverObject->DriverExtension->AddDevice;
#endif                                     /* EMCPAL_USE_CSX_DCALL */
}

//========

#ifdef EMCPAL_KERNEL_STUFF_AVAILABLE

#define EmcpalExtDriverExtensionCreate(Driver,ClientIdentificationAddress,DriverObjectExtensionSize,DriverObjectExtension) \
    EmcpalExtDriverExtensionCreateImpl(Driver,ClientIdentificationAddress,DriverObjectExtensionSize,(PVOID *)(DriverObjectExtension))

/*! @brief EmcpalExtDriverExtensionCreateImpl - Create driver extension
 *  @param DriverObject Driver object
 *  @param ClientIdentificationAddress Client identificatio address
 *  @param DriverObjectExtensionSize Driver extension size
 *  @param DriverObjectExtension Pointer to driver extention
 *  @return STATUS_SUCCESS if success, appropriate error-code otherwise
 */
CSX_STATIC_INLINE EMCPAL_STATUS
EmcpalExtDriverExtensionCreateImpl(
    PEMCPAL_NATIVE_DRIVER_OBJECT DriverObject,
    PVOID ClientIdentificationAddress,
    ULONG DriverObjectExtensionSize,
    PVOID * DriverObjectExtension)
{
#ifdef EMCPAL_USE_CSX_DCALL
    return csx_p_driver_allocate_object_extension(DriverObject, ClientIdentificationAddress, DriverObjectExtensionSize,
        DriverObjectExtension);
#else                                      /* EMCPAL_USE_CSX_DCALL */
    return IoAllocateDriverObjectExtension(DriverObject, ClientIdentificationAddress, DriverObjectExtensionSize, DriverObjectExtension);
#endif                                     /* EMCPAL_USE_CSX_DCALL */
}

/*! @brief EmcpalExtDriverExtensionFind - Find driver extension
 *  @param DriverObject Driver object
 *  @param ClientIdentificationAddress Pointer to client identification address
 *  @return none
 */
CSX_STATIC_INLINE PVOID
EmcpalExtDriverExtensionFind(
    PEMCPAL_NATIVE_DRIVER_OBJECT DriverObject,
    PVOID ClientIdentificationAddress)
{
#ifdef EMCPAL_USE_CSX_DCALL
    return csx_p_driver_get_object_extension(DriverObject, ClientIdentificationAddress);
#else                                      /* EMCPAL_USE_CSX_DCALL */
    return IoGetDriverObjectExtension(DriverObject, ClientIdentificationAddress);
#endif                                     /* EMCPAL_USE_CSX_DCALL */
}

#endif                                     /* EMCPAL_KERNEL_STUFF_AVAILABLE */

//========
/*!
 *	@} end group emcpal_configuration
 *  @} end file EmcPAL_DriverShell_Extensions.h
 */

#endif                                     /* EMCPAL_DRIVER_SHELL_EXTENSIONS_H */
