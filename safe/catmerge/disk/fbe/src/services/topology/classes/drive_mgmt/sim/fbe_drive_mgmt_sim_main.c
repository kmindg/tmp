#define I_AM_NATIVE_CODE
#include <windows.h>
#include "base_object_private.h"
#include "fbe_drive_mgmt_private.h"

fbe_status_t fbe_drive_mgmt_get_esp_lun_location(fbe_char_t *location)
{
    fbe_u32_t pid = 0;
    pid = csx_p_get_process_id();
    fbe_sprintf(location, 256, "./sp_sim_pid%d/dmo_lun_pid%d.txt", pid, pid);
    return FBE_STATUS_OK;
}
