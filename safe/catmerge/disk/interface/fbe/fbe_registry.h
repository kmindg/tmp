#ifndef FBE_REGISTRY_H
#define FBE_REGISTRY_H

#include "fbe/fbe_types.h"
#include "fbe/fbe_service.h"
#include "fbe/fbe_winddk.h"

#define K10_REG_ENV_PATH  "\\REGISTRY\\Machine\\SYSTEM\\CurrentControlSet\\Control\\Session Manager\\Environment"
#define CPD_REG_PATH "\\Registry\\Machine\\system\\CurrentControlSet\\Services\\CPD\\Parameters\\Device"
#define ESP_REG_PATH "\\Registry\\Machine\\system\\CurrentControlSet\\Services\\espkg\\Parameters"
#define NDU_KEY "ReconstructSEPDatabaseFromElias"
#define K10EXPECTEDMEMORYVALUEKEY    "ExpectedMemoryValue"
#define K10SHAREDEXPECTEDMEMORYVALUEKEY "SharedExpectedMemoryValue"
#define SERVICES_REG_PATH "\\Registry\\Machine\\system\\CurrentControlSet\\Services"
#define EspRegPath          "\\REGISTRY\\Machine\\SYSTEM\\CurrentControlSet\\Services\\espkg\\Parameters\\"
#define K10DriverConfigRegPath   "\\REGISTRY\\Machine\\SYSTEM\\CurrentControlSet\\Services\\K10DriverConfig\\"
#define SEP_REG_PATH  "\\REGISTRY\\Machine\\SYSTEM\\CurrentControlSet\\Services\\sep\\Parameters\\"
#define NEW_SP_REG_PATH "\\REGISTRY\\Machine\\SOFTWARE\\wow6432node\\EMC\\K10\\newSP"
#define C4_MIRROR_REG_PATH "\\Registry\\Machine\\System\\CurrentControlSet\\Services\\ntmirror\\Parameters"
#define C4_MIRROR_REINIT_KEY "MDBInit"
#define BOOT_FROM_FLASH_KEY "BootFromFlash"
#define GLOBAL_REG_PATH "\\REGISTRY\\Machine\\SOFTWARE\\EMC\\K10\\Global"
#define SYSTEM_DRIVE_ENCRYPTED "SystemDriveEncrypted"
#define CBE_LICENSE "FIPSEnabled"

//Define Path to Core Config registry path
#define K10_CORE_CONFIG_KEY "\\Registry\\Machine\\System\\CurrentControlSet\\Services\\K10DriverConfig\\CoreConfigRuntime"
#define MAXMEMORYKEYNAME "PlatformMaxFbeMemoryBudgetMB"

#define FBE_REG_BASE_REVISION_KEY          "BASE_REVISION"
#define FBE_REG_BASE_REVISION_KEY_MAX_LEN  128
#define FBE_REG_ADDITIONAL_SUPPORTED_DRIVE_TYPES      "DriveTypesSupportedAdditional"

typedef enum fbe_registry_value_type_e{
	FBE_REGISTRY_VALUE_SZ,              //REG_SZ
	FBE_REGISTRY_VALUE_DWORD,           //REG_DWORD
	FBE_REGISTRY_VALUE_BINARY,          //REG_BINARY
	FBE_REGISTRY_VALUE_MULTISTRING,     //REG_MULTI_SZ
} fbe_registry_value_type_t;



fbe_status_t 
fbe_registry_read(fbe_u8_t* filePathExtraInfo,
				  fbe_u8_t* pRegPath,
				  fbe_u8_t* pKey,
				  void* pBuffer,
				  fbe_u32_t bufferLength,
				  fbe_registry_value_type_t ValueType,
				  void* defaultValue_p,
				  fbe_u32_t defaultLength,
				  fbe_bool_t createKey);
fbe_status_t 
fbe_registry_write(fbe_u8_t* filePathExtraInfo,
				   fbe_u8_t* pRegPath,
				   fbe_u8_t* pKey, 
				   fbe_registry_value_type_t valueType,
				   void   *value_p, 
				   fbe_u32_t  length);
fbe_status_t 
fbe_registry_check(fbe_u32_t RelativeTo,
				   fbe_u8_t *Path);

fbe_status_t fbe_flushRegistry(void);

fbe_status_t fbe_registry_delete_key(fbe_u8_t* pRegPath,
                                     fbe_u8_t* pKey);

#endif
