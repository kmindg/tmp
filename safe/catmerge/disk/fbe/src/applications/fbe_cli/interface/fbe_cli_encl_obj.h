#ifndef FBE_CLI_ENCL_OBJ_H
#define FBE_CLI_ENCL_OBJ_H

#include "fbe/fbe_cli.h"
#include "fbe/fbe_object.h"
#include "fbe/fbe_class.h"
#include "fbe/fbe_types.h"
#include "fbe/fbe_enclosure.h"


#define FBE_CLI_ENCL_FILE_NAME_MAX_LEN 50

typedef enum 
{
    FBE_CLI_ENCL_CMD_TYPE_INVALID,
    FBE_CLI_ENCL_CMD_TYPE_CRU_DEBUG_CTRL,
    FBE_CLI_ENCL_CMD_TYPE_ENCL_DEBUG_CTRL,
    FBE_CLI_ENCL_CMD_TYPE_DRIVE_DEBUG_CTRL,
    FBE_CLI_ENCL_CMD_TYPE_DRIVE_POWER_CTRL,
    FBE_CLI_ENCL_CMD_TYPE_DESTROY_OBJECT,
    FBE_CLI_ENCL_CMD_TYPE_LIST_TRACE_BUF,
    FBE_CLI_ENCL_CMD_TYPE_SAVE_TRACE_BUF,
    FBE_CLI_ENCL_CMD_TYPE_CLEAR_TRACE_BUF,
    FBE_CLI_ENCL_CMD_TYPE_GET_BUF_SIZE,
    FBE_CLI_ENCL_CMD_TYPE_GET_BUF_DATA,
    FBE_CLI_ENCL_CMD_TYPE_EXP_RESET,
    FBE_CLI_ENCL_CMD_TYPE_TEST_MODE,
    FBE_CLI_ENCL_CMD_TYPE_PS_MARGIN_TEST,
    FBE_CLI_ENCL_CMD_TYPE_LAST_ERROR,
    FBE_CLI_ENCL_CMD_TYPE_CONNECTOR_CONTROL
}fbe_cli_encl_cmd_type_t;

void fbe_cli_cmd_display_encl_data(int argc , char ** argv);
void fbe_cli_cmd_enclosure_envchange(int argc , char ** argv);
void fbe_cli_cmd_enclbuf(int argc , char ** argv);
void fbe_cli_cmd_enclosure_info(int argc, char** argv);
void fbe_cli_cmd_enclosure_expStringOut(int argc , char ** argv);
void fbe_cli_cmd_eses_pass_through(int argc , char ** argv);
void fbe_cli_cmd_raw_inquiry(int argc , char ** argv);
void fbe_cli_cmd_enclosure_setled(int argc , char ** argv);
void fbe_cli_cmd_display_dl_info(int argc, char ** argv);
void fbe_cli_cmd_slottophy(int argc, char ** argv);
void fbe_cli_cmd_enclstat(int argc , char ** argv);
void fbe_cli_cmd_enclosure_private_data(int argc, char** argv);
void fbe_cli_cmd_display_statistics(int argc , char ** argv);
void fbe_cli_cmd_enclosure_expStringIn(int argc , char ** argv);
void fbe_cli_cmd_enclosure_expThresholdIn(int argc , char ** argv);
void fbe_cli_cmd_enclosure_expThresholdOut(int argc , char ** argv);
void fbe_cli_cmd_all_enclbuf(int argc , char ** argv);
void fbe_cli_cmd_expander_cmd(int argc , char ** argv);
void fbe_cli_cmd_enclosure_e_log_command(int argc , char ** argv);


#define ENVCHANGE_USAGE           "\
encldbg <-r|a|power_on_off> -b <bus> -e <encl> -s <slot>\n\
      -r                remove disk\n\
      -a                add disk\n\
      -power_on_off <0|1|2>\n\
                        drive power on off command. power_off:0, power_on:1, power_cycle:2 \n\
                        the value for on_off_cycle must be specified\n\
encldbg <-r|a|resetexp|destroy|testmode> -b <bus> -e <encl>\n\
      -r                remove enclosure\n\
      -a                add enclosure\n\
      -resetexp         debug reset Expander\n\
      -testmode [0|1]   show/disable/enable Test Mode\n\
      -destroy          destroy the enclosure object\n\
encldbg <-enable|disable> -b <bus> -e <encl> -conn <connector_id>>\n\
      -enable           enable connector\n\
      -disable          disable connector\n\
\n\
     e.g:\n\
     encldbg -r -b 0 -e 3 -s 2\n\
     encldbg -a -b 0 -e 2\n\
     encldbg -destroy -b 1 -e 2\n\
     encldbg -disable -b 0 -e 3 -conn 0\n\
"

#define ENCLBUF_USAGE           "\
enclbuf -b <port num> -e <encl pos> [-bid <buffer id>] <-listtrace|savetrace|cleartrace|getsize|getdata > filename>\n\
      -listtrace           List the status of all the trace buffers.\n\
      -savetrace           Save the active trace into the oldest buffer or the specified buffer(platform dependent).\n\
      -cleartrace          Clear all the saved trace buffers or the specified buffer(platform dependent). \n\
      -getsize             Get the buffer capacity. Need to specify the trace buffer id.\n\
      -getdata -f filename  Save the buffer data to a file,If file name is not specified display output to stdout. Need to specify the trace buffer id.\n\
"
#define ENCL_REC_DIAG_USAGE           "\
enclrecdiag -b <port num> -e <encl pos> -cmd <cfg|stat|addl_stat|emc_encl>\n\
      -cmd           ESES page.\n\
"
#define ENCL_INQ_USAGE           "\
enclinq -b <port num> -e <encl pos> -cmd <std|sup|unit_sno|dev_id|ext|sub_encl>\n\
      -cmd           SCSI page.\n\
"
#define ENCL_ELEM_GROUP_INFO           "\
enclelemgrp -b <port num> -e <encl pos>\n\
"
#define ENCL_EDAL_USAGE           "\n\
encledal -a Print All EDAL Data including the processor enclosure data.\n\
encledal -pe  Print All Processor Enclosure Component Data\n\
encledal -b <port num> -e <encl pos> [-c <compType> <index>]\n\
    compTypes : ps, drv, lcc, encl, cool, exp, phy, temp (temp sensor), temperature, fw, disp, conn, ssc\n\
encledal -pe -c <peCompType> <index>\n\
    peCompType : ps, cool, platform, misc, iomodule, bem, mezzanine, cachecard, dimm\n\
                 ioport, mgmt, fltexp, fltreg, slaveport, suitcase, mcu, bmc, battery\n\
"
#define EXPSTRINGOUT_USAGE      "\
enclstrout -b <port num> -e <encl pos> -cmd StringOutCommand\n \
    Expamples:\n\
        enclstrout -b 0 -e 0 -cmd prev\n\
        enclstrout -b 0 -e 1 -cmd coolingovr -i 0 -s\n\
"
#define SETLED_USAGE           "\
enclsetled -b <bus num> -e <encl pos> [-d <drive slot> ]\n\
      -list                     display the current state of LED's\n\
      -busdisp <busNumber>      display the Bus Number digit\n\
      -encldisp <enclNumber>    display the Enclosure Number digit\n\
      -a <action>               set drive's led state\n\
      -enclflt <action>         set enclosure's led state\n\
         action: on, off, mark, unmark\n\
    example:\n\
      esl -b 0 -e 0 -list\n\
      esl -b 0 -e 0 -enclflt on\n\
      esl -b 0 -e 0 -encldisp 2\n\
      esl -b 0 -e 0 -d 0 -a mark\n\
"
#define ENCLDLINFO_USAGE      "\
fwdlstatus -b <port num> -e <encl pos> \n\
"
#define SLOTTOPHY_USAGE      "\
slottophy -b <port num> -e <encl pos> -slot <starting slot number> <ending slot number> \n\
"
#define ENCLSTAT_USAGE  "\
enclstat  -shows sumarry for enclosure Data\n\
                usage:  enclstat -h   for help\n\
                        enclstat\n\
"
#define ENCLPRVT_USAGE  "\
enclprvdata  -shows enclosure private data\n\
                usage:  enclprvdata -h   for help\n\
                        enclprvdata\n\
"
#define ENCLSTATISTICS_USAGE  "\
statistics  -display enclsoure statistics\n\
statistics [-h] [-b <port num>] [-e <encl pos>] [-cid <copmponentId>] [-t <element type> [<start slot> [<end slot>]]]\n\
    element types : ps, cool, temp, phy, devslot, exp\n\
    <port num>        Port number of back end bus.\n\
    <encl pos>         Position of enclosure on the back end bus.\n\
    <component Id>  Connector id for internal connections (i.e. Voyager Edge Expanders).\n\
    -h: print help message\n\
"

#define EXPSTRINGIN_USAGE       "\
expstringin -b <port num> -e<encl pos> [-cid <component Id>]\"\n \
    <port num>        Port number of back end bus.\n\
    <encl pos>         Position of enclosure on the back end bus.\n\
    <component Id>  Connector id for internal connections (i.e. Voyager Edge Expanders).\n\
"
#define THRESHOLDIN_USAGE       "\
enclthreshin -b <port num> -e<encl pos> [-cid <component Id>] -t <elem type>\"\n \
    <port num>        Port number of back end bus.\n\
    <encl pos>         Position of enclosure on the back end bus.\n\
    <component Id>  Connector id for internal connections (i.e. Voyager Edge Expanders).\n\
"

#define EXPTHRESHOLDOUT_USAGE           "\
enclthreshout     -b <port num> -e <encl pos> -et <element type> \n\
                  -hct [high critical threshold] -hwt [high warning threshold] \n\
                  -lwt [low warning threshold] -lct [low critical threshold]\n\
                  for temp overall element lwt and lct should be 0. \n\
examples:\n\
        enclthreshout -b 0 -e 1 -et temp -hct 60 -hwt 46 -lwt 0 -lct 0 \n"

#define ENCLCOLLECT_ALL_USAGE  "\
collectall  -shows output for differnt fbe_cli commands\n\
                usage:  collectall -h   for help\n\
                        collectall\n\
"

#define   ALLENCLBUF_USAGE    "\
allenclbuf [-clear]\n"

#define EXPCMDS_USAGE  "\
expcmds  - executes expander cmds(pphy,pdrv,pinfo,pdevices) \n\
usage:  expcmds -h   For help information.\n\
        expcmds [-b <port_num>][ -e <encl_pos>][-cid <component Id>]  \n\
                      -executes expanders cmds for all buses and enclosures.\n\
    <port num>        Port number of back end bus.\n\
    <encl pos>         Position of enclosure on the back end bus.\n\
    <component Id>  Connector id for internal connections (i.e. Voyager Edge Expanders).\n\
"

#define EVENTLOG_USAGE  "\
eventlog  -show e-log trace buffer\n\
                usage:  eventlog -h   for help\n\
                        eventlog\n\
"

#endif /* FBE_CLI_ENCL_OBJ */
