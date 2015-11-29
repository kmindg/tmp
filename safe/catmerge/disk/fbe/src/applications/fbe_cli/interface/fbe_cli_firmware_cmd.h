#ifndef FBE_CLI_FIRMWARE_CMD_H
#define FBE_CLI_FIRMWARE_CMD_H


/*************************
 * INCLUDE FILES
 *************************/
#include "fbe/fbe_cli.h"
#include "fbe/fbe_types.h"
#include "fbe_devices.h"

/*************************
 * PROTOTYPES
 *************************/
void fbe_cli_cmd_firmware_upgrade(fbe_s32_t argc,char ** argv);
fbe_status_t fbe_cli_get_fup_status(fbe_u64_t deviceType, fbe_device_physical_location_t *pLocation);
fbe_status_t fbe_cli_get_fup_revision(fbe_u64_t deviceType, fbe_device_physical_location_t *pLocation, fbe_bool_t eng_option);

#define FUP_USAGE "\
firmwareUpgrade/fup  Show all Firmware Upgrade  related information \n\
Usage: fup -h \n\
fup <switches> \n\
Switches: \n\
    <i> -d <device type> [-b <bus num>] [-e <encl num>] [-s <slot num>] [-cid <component ID>] [-f <force option>] \n\
        Initiate firmware upgrade for the device in the specified location. \n\
        -d Device Type:lcc, bm, ps, sps, sp or fan. This parameter is required. \n\
        -b Bus number \n\
        -e Enclosure number \n\
        -s slot number (within the enclosure) \n\
        -cid component ID (Only valid for Voyager EE LCC, SPS) \n\
        -sp SP number (0 or 1). Needed for device type bm and sp\n\
        -f force options (norevcheck , singlesp, noenvcheck, readimage, readmanifestfile, noprioritycheck) \n\
           norevcheck - upgrade without checking the current revision \n\
           singlesp - peer sp not required to be present \n\
           noenvcheck - upgrade without check environment conditions \n\
           readimage - force read of the image file \n\
           readmanifestfile - force read of the manifest file \n\
           noprioritycheck - no component precendant checking \n\
           nobadimagecheck - proceed even if the image is marked as bad \n\
        If device location is not specified, initiate upgrade for all devices with the specified type. \n\
        If firmware upgrade is on xPE, the bus and enclosure number should be specified or shown as 254. \n\
    <s> -d <device type> [-b <bus num>] [-e <encl num>] [-s <slot num>] \n\
        Display status informaiton for firmware upgrades. \n\
        -d Device Type:lcc, bm, ps, sps, sp or fan. This parameter is required. \n\
        -b Bus number \n\
        -e Enclosure number \n\
        -s slot number (within the enclosure) \n\
        -cid component ID (Only valid for Voyager EE LCC, SPS) \n\
        -sp SP number (0 or 1). Needed for device type bm and sp\n\
        If device location is not specified, display status for all devices with the specified type. \n\
        If firmware upgrade is on xPE, the bus and enclosure number should be specified or shown as 254. \n\
    <r> -d <device type> [-b <bus num>] [-e <encl num>] [-s <slot num>] \n\
        Display the firmware revision. \n\
        -d Device Type:lcc, bm, ps, sps, sp or fan. This parameter is required. \n\
        -b Bus number \n\
        -e Enclosure number \n\
        -s slot number (within the enclosure) \n\
        -cid component ID (Only valid for Voyager EE LCC, SPS) \n\
        -sp SP number (0 or 1). Needed for device type bm and sp\n\
        If device location is not specified, display status for all devices with the specified type. \n\
        If firmware upgrade is on xPE, the bus and enclosure number should be specified or shown as 254. \n\
    <terminate> \n\
        - terminate all the upgrade currently in the queues for all the device types. \n\
    <abort> \n\
        - Abort all the upgrade for all the device types. Will not start the upgrade until resume is received. \n\
    <resume> \n\
        - Resume all the upgrade for all the device types. \n\
    <m> -d <device type> \n\
        Display the manifest info for the specified device type. \n\
        -d Device Type:lcc, bm, ps, sps, sp or fan. This parameter is required. \n\
    <t> -d <device type> -delay <delayInSeconds> \n\
        Modify the firmware upgrade delay left time for the specified device type. \n\
         -d Device Type:lcc, bm, ps, sps, sp or fan. This parameter is required. \n\
         -delay The firmware upgrade delay time(in seconds) from now. Use 0 to start upgrade right away. \n\
Examples: \n\
    fup i -d ps -b 0 -e 1 -s 0    : Upgrade power supply in enclosure 1 \n\
    fup i -d lcc                  : Upgrade all lccs \n\
    fup i -d sp -b 0 -e 0 -s 0 -sp 1        : Upgrade SP board B\n\
    fup i -d bm -b 0 -e 0 -s 0 -sp 0        : Upgrade Base Module on SPA\n\
    fup i -d lcc -b 0 -e 0 -s 2 -cid 4      : Upgrade one expander on lcc 2 \n\
    fup i -d lcc -f norevcheck -f singlesp  : Force upgrade of all lccs, single sp \n\
    fup s -d fan                  : display upgrade status for all fans \n\
    fup s -d sps -b 254 -e 254    : display upgrade status for xPE SPS \n\
    fup r -d -b 1 -e 3 -s 2       : display firmware rev for fan 2 \n\
    fup m -d lcc                  : display the LCC manifest info \n\
    fup t -d lcc -delay 300       : LCC firmware upgrade will start in 300s \n\
    fup t -d lcc -delay 0         : LCC firmware upgrade will start now \n"


#endif /* FBE_CLI_FIRMWARE_CMD_H */
