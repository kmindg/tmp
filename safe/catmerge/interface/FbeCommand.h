#ifndef FBE_COMMAND_H
#define FBE_COMMAND_H


#include "k10defs.h"
#include "FlareCommand.h"

#include "fbe/fbe_lun.h"
#include "fbe/fbe_api_database_interface.h"

class FbeCommandImpl;
class FbeData;

typedef enum obj
{
   OBJ_RG =  0,
   OBJ_LUN
} OBJECT;

typedef enum op_command
{
   CREATE_RG = 0,
   REMOVE_RG,
   BIND,
   UNBIND,
   LUN_CONFIG,
   RG_CONFIG
} OP_COMMAND;

class FbeCommand
{
public:
	FbeCommand(FbeData *pFbeData);
	~FbeCommand();
	
	long OnlineDriveFirmwareUpgrade_Cancel();
	long OnlineDriveFirmwareUpgrade_Trial_Start(AdmOdfuConfig *cmd);
    long Verify(K10_WWID &lunKey, BOOL bClear );
    long VerifyStatus (K10_WWID &lunKey, fbe_lun_get_verify_status_t *status);
    long VerifyReport(K10_WWID& wwn, fbe_lun_verify_report_t *report );
    long GroupVerify( const fbe_u32_t &rg, BOOL bClear);
    long SystemVerify(BOOL bClear  );
    long DiskVerify( const BOOL& bDiskVerify );

	long Bind(AdmLunBindData *bindInfo, unsigned long &lun_number);

	long UnBind(K10_LU_ID wwnLun);

	long LunConfigBind( ADM_UNIT_CONFIG_CMD *cmd );
	long LunConfig( ADM_UNIT_CONFIG_CMD *cmd );

	long SystemConfig( ADM_SYSTEM_CONFIG_CMD *cmd );
	long SetQueueDepth(CM_QUEUE_DEPTH_OP type_of_op, DWORD depth);
	long SetNDUState(ADM_SET_NDU_STATE_CMD *pNDUState);
	long SetNDUInProgress(const BOOL& bInProgress);
	long PowerSavingsConfig( ADM_POWER_SAVING_CONFIG_CMD *cmd);
	long GetCompatMode(K10_COMPATIBILITY_MODE_WRAP * pWrap);
	long Commit();
	long UpdatePoolId(const AdmDiskPoolParams &dpParams);
	long DestroyDiskPool(long pool_id);
	long ZeroDisk(const K10_WWID &diskKey, BOOL bStop);
	long BackDoorPower(ADM_BACKDOOR_POWER_CMD *backDoor);
	long BackDoorDiskPower(const K10_WWID &DiskWwn, ADM_FRU_POWER power_on, BOOL defer);
	long EnclosureConfig(AdmFlashControl *cmd);
	long IOMPortConfig(ADM_LED_CTRL_CMD *cmd, BOOL bIOM);
	long IOMSlicUpgrade( ADM_SET_SLIC_UPGRADE_DATA *cmd );
	long FlexportConfig(ADM_PORT_CONFIG_DATA *cmd);
	long ManagementPortSpeeds(ADM_CONFIG_MGMT_PORTS_CMD *cmd);
	long RaidGroupConfig(AdmRaidGroupConfigCmd *cmd);
	long RaidGroupBindConfig(ADM_BIND_GET_CONFIG_CMD *pCmd, ADM_BIND_GET_CONFIG_DATA *pStatus);
    long StartProactiveSpare( const K10_WWID& DiskWwn );
    long StartDiskCopyTo( K10_WWID& src, K10_WWID& dst );
	long Reboot( ADM_REBOOT_SP_CMD *cmd );
	long Shutdown();
	long JobStatus(fbe_job_service_error_type_t job_status, int object, int op);
	long CopyToDiskStatus(fbe_provision_drive_copy_to_status_t status);
	long SpPanic();
	long EstablishKEKKEK(fbe_u64_t& encryptionNonce);
	long ActivateEncryption(fbe_u64_t encryptionNonce);
	long InitiateBackup(char* encryptionBackupFilename);
	long NotifyBackupFileRetrieved(const char* backupFilename);
	long GetAuditLogFile(fbe_u32_t year, fbe_u32_t month, char* auditLogFilename);
	long NotifyAuditLogFileRetrieved(const char* auditLogFilename);
	long GetCSumFile(const char* auditLogFilename, char * csumFile);
	long NotifyCSumFileRetrieved(const char* csumFile);
	long GetActivationStatus(fbe_u64_t& activationStatus);
private:
	FbeCommandImpl *mpImpl;
};

#endif
