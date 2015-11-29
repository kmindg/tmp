/*
 * place copyright statement here -- 2000
 */

/******************************************************/
/* G E N E R A T E D   F I L E -- D o  n o t  E D I T */
/* alter trc_tree.txt and/or trc_text.txt and regen   */
/******************************************************/

/*
 * generated by tree2envh.c compiled on Apr 19 2000
 * from files /TBS/ /TBS/
 */

#if !defined(TRC_ENVIRON2_H)
#define      TRC_ENVIRON2_H

#include "csx_ext.h"

#if !defined(NULL)
#define NULL 000
#endif

typedef struct TRC_comp_struct
{
      TRC_flag_T*      flag_P;
      TRC_flag_T       possible;
      char*            name;
      TRC_bit_name_T*  bit_names_P;
} TRC_comp_T;

#pragma data_seg ("TRC_TABLE$4strings")
static char CSX_MAYBE_UNUSED str_NULL[] = "*NULL*";
static char CSX_MAYBE_UNUSED str_TRC_CM[] = "CM facility (configuration management)";
static char CSX_MAYBE_UNUSED str_TRC_EVIL[] = "EVIL test for added types";
static char CSX_MAYBE_UNUSED str_TRC_TRC[] = "TRC function (display and store in ring buffers";
#pragma data_seg ("TRC_TABLE$2elements")

#if defined (GEN_DATA)
#if defined (TRC_GEN_CONTROL)
#ifndef ALAMOSA_WINDOWS_ENV
static TRC_comp_T CSX_MAYBE_UNUSED TRC_comps_array[] =
#else
extern TRC_comp_T TRC_comps_array[] =
#endif /* ALAMOSA_WINDOWS_ENV - ODDCASE - compiler quirks */
#else
static TRC_comp_T CSX_MAYBE_UNUSED TRC_comps_array[] =
#endif
{
#if defined(TRC_GROUP_ADDSLAVE)
{&TRC_ADDSLAVE_flag,
 (TRC_flag_T)TRC_ADDSLAVE_FLAG,
 str_NULL,
 000
},
#endif

#if defined(TRC_GROUP_ADMDISP)
{&TRC_ADMDISP_flag,
 (TRC_flag_T)TRC_ADMDISP_FLAG,
 str_NULL,
 000
},
#endif

#if defined(TRC_GROUP_ADMENG)
{&TRC_ADMENG_flag,
 (TRC_flag_T)TRC_ADMENG_FLAG,
 str_NULL,
 000
},
#endif

#if defined(TRC_GROUP_1SHOT)
{&TRC_1SHOT_flag,
 (TRC_flag_T)TRC_1SHOT_FLAG,
 str_NULL,
 000
},
#endif

#if defined(TRC_GROUP_BINDOBJ)
{&TRC_BINDOBJ_flag,
 (TRC_flag_T)TRC_BINDOBJ_FLAG,
 str_NULL,
 000
},
#endif

#if defined(TRC_GROUP_BLOW)
{&TRC_BLOW_flag,
 (TRC_flag_T)TRC_BLOW_FLAG,
 str_NULL,
 000
},
#endif

#if defined(TRC_GROUP_CDS)
{&TRC_CDS_flag,
 (TRC_flag_T)TRC_CDS_FLAG,
 str_NULL,
 000
},
#endif

#if defined(TRC_GROUP_CHGSLAVE)
{&TRC_CHGSLAVE_flag,
 (TRC_flag_T)TRC_CHGSLAVE_FLAG,
 str_NULL,
 000
},
#endif

#if defined(TRC_GROUP_CHKIMG)
{&TRC_CHKIMG_flag,
 (TRC_flag_T)TRC_CHKIMG_FLAG,
 str_NULL,
 000
},
#endif

#if defined(TRC_GROUP_CONFIG)
{&TRC_CONFIG_flag,
 (TRC_flag_T)TRC_CONFIG_FLAG,
 str_NULL,
 000
},
#endif

#if defined(TRC_GROUP_FEDISK)
{&TRC_FEDISK_flag,
 (TRC_flag_T)TRC_FEDISK_FLAG,
 str_NULL,
 000
},
#endif

#if defined(TRC_GROUP_CPM)
{&TRC_CPMCOPY_flag,
 (TRC_flag_T)TRC_CPMCOPY_FLAG,
 str_NULL,
 000
},
#endif

#if defined(TRC_GROUP_CPM)
{&TRC_CPMCTRL_flag,
 (TRC_flag_T)TRC_CPMCTRL_FLAG,
 str_NULL,
 000
},
#endif

#if defined(TRC_GROUP_CPM)
{&TRC_CPMDRVR_flag,
 (TRC_flag_T)TRC_CPMDRVR_FLAG,
 str_NULL,
 000
},
#endif

#if defined(TRC_GROUP_CPM)
{&TRC_CPMGSKT_flag,
 (TRC_flag_T)TRC_CPMGSKT_FLAG,
 str_NULL,
 000
},
#endif

/* #if defined(TRC_GROUP_CPM)
{&TRC_CPMLIB_flag,
 (TRC_flag_T)TRC_CPMLIB_FLAG,
 str_NULL,
 000
},
#endif */

#if defined(TRC_GROUP_CPM)
{&TRC_CPMMGMT_flag,
 (TRC_flag_T)TRC_CPMMGMT_FLAG,
 str_NULL,
 000
},
#endif

#if defined(TRC_GROUP_CPM)
{&TRC_CPMPARSE_flag,
 (TRC_flag_T)TRC_CPMPARSE_FLAG,
 str_NULL,
 000
},
#endif

#if defined(TRC_GROUP_CPM)
{&TRC_CPMPROC_flag,
 (TRC_flag_T)TRC_CPMPROC_FLAG,
 str_NULL,
 000
},
#endif

#if defined(TRC_GROUP_CREATLNK)
{&TRC_CREATLNK_flag,
 (TRC_flag_T)TRC_CREATLNK_FLAG,
 str_NULL,
 000
},
#endif

#if defined(TRC_GROUP_CREATMIR)
{&TRC_CREATMIR_flag,
 (TRC_flag_T)TRC_CREATMIR_FLAG,
 str_NULL,
 000
},
#endif

#if defined(TRC_GROUP_DAQ)
{&TRC_DAQ_flag,
 (TRC_flag_T)TRC_DAQ_FLAG,
 str_NULL,
 000
},
#endif

#if defined(TRC_GROUP_DESTMIR)
{&TRC_DESTMIR_flag,
 (TRC_flag_T)TRC_DESTMIR_FLAG,
 str_NULL,
 000
},
#endif

#if defined(TRC_GROUP_DGINT)
{&TRC_DGINT_flag,
 (TRC_flag_T)TRC_DGINT_FLAG,
 str_NULL,
 000
},
#endif

#if defined(TRC_GROUP_DISKTEST)
{&TRC_DISKTEST_flag,
 (TRC_flag_T)TRC_DISKTEST_FLAG,
 str_NULL,
 000
},
#endif

#if defined(TRC_GROUP_DLS1LOCK)
{&TRC_DLS1LOCK_flag,
 (TRC_flag_T)TRC_DLS1LOCK_FLAG,
 str_NULL,
 000
},
#endif

#if defined(TRC_GROUP_DLSDRVR)
{&TRC_DLSDRVR_flag,
 (TRC_flag_T)TRC_DLSDRVR_FLAG,
 str_NULL,
 000
},
#endif

#if defined(TRC_GROUP_DLSEXP)
{&TRC_DLSEXP_flag,
 (TRC_flag_T)TRC_DLSEXP_FLAG,
 str_NULL,
 000
},
#endif

#if defined(TRC_GROUP_DLS)
{&TRC_DLSEXT_flag,
 (TRC_flag_T)TRC_DLSEXT_FLAG,
 str_NULL,
 000
},
#endif

#if defined(TRC_GROUP_DLSUTIL)
{&TRC_DLSUTIL_flag,
 (TRC_flag_T)TRC_DLSUTIL_FLAG,
 str_NULL,
 000
},
#endif

#if defined(TRC_GROUP_DUMPMAN)
{&TRC_DUMPMAN_flag,
 (TRC_flag_T)TRC_DUMPMAN_FLAG,
 str_NULL,
 000
},
#endif

#if defined(TRC_GROUP_ENDPT)
{&TRC_ENDPT_flag,
 (TRC_flag_T)TRC_ENDPT_FLAG,
 str_NULL,
 000
},
#endif

#if defined(TRC_GROUP_ENUMMIR)
{&TRC_ENUMMIR_flag,
 (TRC_flag_T)TRC_ENUMMIR_FLAG,
 str_NULL,
 000
},
#endif

#if defined(TRC_GROUP_ENUMOBJS)
{&TRC_ENUMOBJS_flag,
 (TRC_flag_T)TRC_ENUMOBJS_FLAG,
 str_NULL,
 000
},
#endif

#if defined(TRC_GROUP_FOO)
{&TRC_EVIL_flag,
 (TRC_evil_T)TRC_EVIL_FLAG,
 str_TRC_EVIL,
 000
},
#endif

#if defined(TRC_GROUP_FAKELC)
{&TRC_FAKELC_flag,
 (TRC_flag_T)TRC_FAKELC_FLAG,
 str_NULL,
 000
},
#endif

#if defined(TRC_GROUP_FLIPTEST)
{&TRC_FLIPTEST_flag,
 (TRC_flag_T)TRC_FLIPTEST_FLAG,
 str_NULL,
 000
},
#endif

#if defined(TRC_GROUP_FRACSYNC)
{&TRC_FRACSYNC_flag,
 (TRC_flag_T)TRC_FRACSYNC_FLAG,
 str_NULL,
 000
},
#endif

#if defined(TRC_GROUP_GETDRVR)
{&TRC_GETDRVR_flag,
 (TRC_flag_T)TRC_GETDRVR_FLAG,
 str_NULL,
 000
},
#endif

#if defined(TRC_GROUP_GETGEN)
{&TRC_GETGEN_flag,
 (TRC_flag_T)TRC_GETGEN_FLAG,
 str_NULL,
 000
},
#endif

#if defined(TRC_GROUP_GETIMG)
{&TRC_GETIMG_flag,
 (TRC_flag_T)TRC_GETIMG_FLAG,
 str_NULL,
 000
},
#endif

#if defined(TRC_GROUP_GETMIR)
{&TRC_GETMIR_flag,
 (TRC_flag_T)TRC_GETMIR_FLAG,
 str_NULL,
 000
},
#endif

#if defined(TRC_GROUP_GETSPIDS)
{&TRC_GETSPIDS_flag,
 (TRC_flag_T)TRC_GETSPIDS_FLAG,
 str_NULL,
 000
},
#endif

#if defined(TRC_GROUP_HASH)
{&TRC_HASH_flag,
 (TRC_flag_T)TRC_HASH_FLAG,
 str_NULL,
 000
},
#endif

#if defined(TRC_GROUP_HOSTTOOLS)
{&TRC_HOSTTOOLS_flag,
 (TRC_flag_T)TRC_HOSTTOOLS_FLAG,
 str_NULL,
 000
},
#endif

#if defined(TRC_GROUP_K10ADMIN)
{&TRC_K10ADMIN_flag,
 (TRC_flag_T)TRC_K10ADMIN_FLAG,
 str_NULL,
 000
},
#endif

#if defined(TRC_GROUP_K10APP)
{&TRC_K10APP_flag,
 (TRC_flag_T)TRC_K10APP_FLAG,
 str_NULL,
 000
},
#endif

#if defined(TRC_GROUP_K10DRVRS)
{&TRC_K10DRVRS_flag,
 (TRC_flag_T)TRC_K10DRVRS_FLAG,
 str_NULL,
 000
},
#endif

#if defined(TRC_GROUP_K10ELIB)
{&TRC_K10ELIB_flag,
 (TRC_flag_T)TRC_K10ELIB_FLAG,
 str_NULL,
 000
},
#endif

#if defined(TRC_GROUP_K10FALIB)
{&TRC_K10FALIB_flag,
 (TRC_flag_T)TRC_K10FALIB_FLAG,
 str_NULL,
 000
},
#endif

#if defined(TRC_GROUP_K10GLOB)
{&TRC_K10GLOB_flag,
 (TRC_flag_T)TRC_K10GLOB_FLAG,
 str_NULL,
 000
},
#endif

#if defined(TRC_GROUP_K10GOV)
{&TRC_K10GOV_flag,
 (TRC_flag_T)TRC_K10GOV_FLAG,
 str_NULL,
 000
},
#endif

#if defined(TRC_GROUP_K10HALIB)
{&TRC_K10HALIB_flag,
 (TRC_flag_T)TRC_K10HALIB_FLAG,
 str_NULL,
 000
},
#endif

#if defined(TRC_GROUP_K10INSDRVR)
{&TRC_K10INSDRVR_flag,
 (TRC_flag_T)TRC_K10INSDRVR_FLAG,
 str_NULL,
 000
},
#endif

#if defined(TRC_GROUP_K10KILL)
{&TRC_K10KILL_flag,
 (TRC_flag_T)TRC_K10KILL_FLAG,
 str_NULL,
 000
},
#endif

#if defined(TRC_GROUP_K10KRNLMSG)
{&TRC_K10KRNLMSG_flag,
 (TRC_flag_T)TRC_K10KRNLMSG_FLAG,
 str_NULL,
 000
},
#endif

#if defined(TRC_GROUP_K10MISCUTIL)
{&TRC_K10MISCUTIL_flag,
 (TRC_flag_T)TRC_K10MISCUTIL_FLAG,
 str_NULL,
 000
},
#endif

#if defined(TRC_GROUP_K10PALIB)
{&TRC_K10PALIB_flag,
 (TRC_flag_T)TRC_K10PALIB_FLAG,
 str_NULL,
 000
},
#endif

#if defined(TRC_GROUP_K10SETUP)
{&TRC_K10SETUP_flag,
 (TRC_flag_T)TRC_K10SETUP_FLAG,
 str_NULL,
 000
},
#endif

#if defined(TRC_GROUP_K10SYSLIB)
{&TRC_K10SYSLIB_flag,
 (TRC_flag_T)TRC_K10SYSLIB_FLAG,
 str_NULL,
 000
},
#endif

#if defined(TRC_GROUP_KFIAGENT)
{&TRC_KFIAGENT_flag,
 (TRC_flag_T)TRC_KFIAGENT_FLAG,
 str_NULL,
 000
},
#endif

#if defined(TRC_GROUP_KFID)
{&TRC_KFID_flag,
 (TRC_flag_T)TRC_KFID_FLAG,
 str_NULL,
 000
},
#endif

#if defined(TRC_GROUP_KFIDRVR)
{&TRC_KFIDRVR_flag,
 (TRC_flag_T)TRC_KFIDRVR_FLAG,
 str_NULL,
 000
},
#endif

#if defined(TRC_GROUP_KFIINSDRVR)
{&TRC_KFIINSDRVR_flag,
 (TRC_flag_T)TRC_KFIINSDRVR_FLAG,
 str_NULL,
 000
},
#endif

#if defined(TRC_GROUP_KFIMISC)
{&TRC_KFIMISC_flag,
 (TRC_flag_T)TRC_KFIMISC_FLAG,
 str_NULL,
 000
},
#endif

#if defined(TRC_GROUP_KFIPIPE)
{&TRC_KFIPIPE_flag,
 (TRC_flag_T)TRC_KFIPIPE_FLAG,
 str_NULL,
 000
},
#endif

#if defined(TRC_GROUP_KFITEST)
{&TRC_KFITEST_flag,
 (TRC_flag_T)TRC_KFITEST_FLAG,
 str_NULL,
 000
},
#endif

#if defined(TRC_GROUP_KFIUCLS)
{&TRC_KFIUCLS_flag,
 (TRC_flag_T)TRC_KFIUCLS_FLAG,
 str_NULL,
 000
},
#endif

#if defined(TRC_GROUP_KMODE)
{&TRC_KMODE_flag,
 (TRC_flag_T)TRC_KMODE_FLAG,
 str_NULL,
 000
},
#endif

#if defined(TRC_GROUP_KTRACE)
{&TRC_KTRACE_flag,
 (TRC_flag_T)TRC_KTRACE_FLAG,
 str_NULL,
 000
},
#endif

#if defined(TRC_GROUP_MEM)
{&TRC_MEM_flag,
 (TRC_flag_T)TRC_MEM_FLAG,
 str_NULL,
 000
},
#endif

#if defined(TRC_GROUP_MGMTCOM)
{&TRC_MGMTCOM_flag,
 (TRC_flag_T)TRC_MGMTCOM_FLAG,
 str_NULL,
 000
},
#endif

#if defined(TRC_GROUP_MINPTAPI)
{&TRC_MINPTAPI_flag,
 (TRC_flag_T)TRC_MINPTAPI_FLAG,
 str_NULL,
 000
},
#endif

#if defined(TRC_GROUP_MINPTCOM)
{&TRC_MINPTCOM_flag,
 (TRC_flag_T)TRC_MINPTCOM_FLAG,
 str_NULL,
 000
},
#endif

#if defined(TRC_GROUP_MINPTFCB)
{&TRC_MINPTFCB_flag,
 (TRC_flag_T)TRC_MINPTFCB_FLAG,
 str_NULL,
 000
},
#endif

#if defined(TRC_GROUP_MINPTFCF)
{&TRC_MINPTFCF_flag,
 (TRC_flag_T)TRC_MINPTFCF_FLAG,
 str_NULL,
 000
},
#endif

#if defined(TRC_GROUP_MINPTHPTL)
{&TRC_MINPTHPTL_flag,
 (TRC_flag_T)TRC_MINPTHPTL_FLAG,
 str_NULL,
 000
},
#endif

#if defined(TRC_GROUP_MINPTTEST)
{&TRC_MINPTTEST_flag,
 (TRC_flag_T)TRC_MINPTTEST_FLAG,
 str_NULL,
 000
},
#endif

#if defined(TRC_GROUP_MODDBG)
{&TRC_MODDBG_flag,
 (TRC_flag_T)TRC_MODDBG_FLAG,
 str_NULL,
 000
},
#endif

#if defined(TRC_GROUP_MPENG)
{&TRC_MPENG_flag,
 (TRC_flag_T)TRC_MPENG_FLAG,
 str_NULL,
 000
},
#endif

#if defined(TRC_GROUP_MPS)
{&TRC_MPS_flag,
 (TRC_flag_T)TRC_MPS_FLAG,
 str_NULL,
 000
},
#endif

#if defined(TRC_GROUP_MPSBAD)
{&TRC_MPSBAD_flag,
 (TRC_flag_T)TRC_MPSBAD_FLAG,
 str_NULL,
 000
},
#endif

#if defined(TRC_GROUP_MSPCNT)
{&TRC_MPSCNT_flag,
 (TRC_flag_T)TRC_MPSCNT_FLAG,
 str_NULL,
 000
},
#endif

#if defined(TRC_GROUP_MPSDLL)
{&TRC_MPSDLL_flag,
 (TRC_flag_T)TRC_MPSDLL_FLAG,
 str_NULL,
 000
},
#endif

#if defined(TRC_GROUP_MPSKCNT)
{&TRC_MPSKCNT_flag,
 (TRC_flag_T)TRC_MPSKCNT_FLAG,
 str_NULL,
 000
},
#endif

#if defined(TRC_GROUP_MPSKSTRESS)
{&TRC_MPSKSTRESS_flag,
 (TRC_flag_T)TRC_MPSKSTRESS_FLAG,
 str_NULL,
 000
},
#endif

#if defined(TRC_GROUP_MPSOPEN)
{&TRC_MPSOPEN_flag,
 (TRC_flag_T)TRC_MPSOPEN_FLAG,
 str_NULL,
 000
},
#endif

#if defined(TRC_GROUP_MPSSEND)
{&TRC_MPSSEND_flag,
 (TRC_flag_T)TRC_MPSSEND_FLAG,
 str_NULL,
 000
},
#endif

#if defined(TRC_GROUP_MPSUCNT)
{&TRC_MPSUCNT_flag,
 (TRC_flag_T)TRC_MPSUCNT_FLAG,
 str_NULL,
 000
},
#endif

#if defined(TRC_GROUP_MPSUMPS)
{&TRC_MPSUMPS_flag,
 (TRC_flag_T)TRC_MPSUMPS_FLAG,
 str_NULL,
 000
},
#endif

#if defined(TRC_GROUP_MPSUSTRESS)
{&TRC_MPSUSTRESS_flag,
 (TRC_flag_T)TRC_MPSUSTRESS_FLAG,
 str_NULL,
 000
},
#endif

#if defined(TRC_GROUP_MPTESTAPP)
{&TRC_MPTESTAPP_flag,
 (TRC_flag_T)TRC_MPTESTAPP_FLAG,
 str_NULL,
 000
},
#endif

#if defined(TRC_GROUP_MSGDISP)
{&TRC_MSGDISP_flag,
 (TRC_flag_T)TRC_MSGDISP_FLAG,
 str_NULL,
 000
},
#endif

#if defined(TRC_GROUP_NDU)
{&TRC_NDU_flag,
 (TRC_flag_T)TRC_NDU_FLAG,
 str_NULL,
 000
},
#endif

#if defined(TRC_GROUP_NDU)
{&TRC_NDUADMLIB_flag,
 (TRC_flag_T)TRC_NDUADMLIB_FLAG,
 str_NULL,
 000
},
#endif

#if defined(TRC_GROUP_NDU)
{&TRC_NDULIB_flag,
 (TRC_flag_T)TRC_NDULIB_FLAG,
 str_NULL,
 000
},
#endif

#if defined(TRC_GROUP_NDUMON)
{&TRC_NDUMON_flag,
 (TRC_flag_T)TRC_NDUMON_FLAG,
 str_NULL,
 000
},
#endif

#if defined(TRC_GROUP_NDUSETUP)
{&TRC_NDUSETUP_flag,
 (TRC_flag_T)TRC_NDUSETUP_FLAG,
 str_NULL,
 000
},
#endif

#if defined(TRC_GROUP_NDUUPG)
{&TRC_NDUUPG_flag,
 (TRC_flag_T)TRC_NDUUPG_FLAG,
 str_NULL,
 000
},
#endif

#if defined(TRC_GROUP_NVRAM)
{&TRC_NVRAMDRV_flag,
 (TRC_flag_T)TRC_NVRAMDRV_FLAG,
 str_NULL,
 000
},
#endif

#if defined(TRC_GROUP_NVRAM)
{&TRC_NVRAMLIB_flag,
 (TRC_flag_T)TRC_NVRAMLIB_FLAG,
 str_NULL,
 000
},
#endif

#if defined(TRC_GROUP_PCMI)
{&TRC_PCMICORE_flag,
 (TRC_flag_T)TRC_PCMICORE_FLAG,
 str_NULL,
 000
},
#endif

#if defined(TRC_GROUP_PCMI)
{&TRC_PCMIDRVR_flag,
 (TRC_flag_T)TRC_PCMIDRVR_FLAG,
 str_NULL,
 000
},
#endif

#if defined(TRC_GROUP_PCMI)
{&TRC_PCMIKMODE_flag,
 (TRC_flag_T)TRC_PCMIKMODE_FLAG,
 str_NULL,
 000
},
#endif

#if defined(TRC_GROUP_PCMI)
{&TRC_PCMITHREAD_flag,
 (TRC_flag_T)TRC_PCMITHREAD_FLAG,
 str_NULL,
 000
},
#endif

#if defined(TRC_GROUP_PCMI)
{&TRC_PCMIUMODE_flag,
 (TRC_flag_T)TRC_PCMIUMODE_FLAG,
 str_NULL,
 000
},
#endif

#if defined(TRC_GROUP_PERF)
{&TRC_PERF_flag,
 (TRC_flag_T)TRC_PERF_FLAG,
 str_NULL,
 000
},
#endif

#if defined(TRC_GROUP_PERF)
{&TRC_PERF_flag,
 (TRC_flag_T)TRC_PERF_FLAG,
 str_TRC_NULL,
 000
},
#endif

#if defined(TRC_GROUP_PERFTEST)
{&TRC_PERFTEST_flag,
 (TRC_flag_T)TRC_PERFTEST_FLAG,
 str_NULL,
 000
   },
#endif

#if defined(TRC_GROUP_PRINTDB)
{&TRC_PRINTDB_flag,
 (TRC_flag_T)TRC_PRINTDB_FLAG,
 str_NULL,
 000
},
#endif

#if defined(TRC_GROUP_PRINTSTAT)
{&TRC_PRINTSTAT_flag,
 (TRC_flag_T)TRC_PRINTSTAT_FLAG,
 str_NULL,
 000
},
#endif

#if defined(TRC_GROUP_PROMOTE)
{&TRC_PROMOTE_flag,
 (TRC_flag_T)TRC_PROMOTE_FLAG,
 str_NULL,
 000
},
#endif

#if defined(TRC_GROUP_PSMADMIN)
{&TRC_PSMADMIN_flag,
 (TRC_flag_T)TRC_PSMADMIN_FLAG,
 str_NULL,
 000
},
#endif

#if defined(TRC_GROUP_PSMEXT)
{&TRC_PSMEXT_flag,
 (TRC_flag_T)TRC_PSMEXT_FLAG,
 str_NULL,
 000
},
#endif

#if defined(TRC_GROUP_PSMMENU)
{&TRC_PSMMENU_flag,
 (TRC_flag_T)TRC_PSMMENU_FLAG,
 str_NULL,
 000
},
#endif

#if defined(TRC_GROUP_PSMREGRESS)
{&TRC_PSMREGRESS_flag,
 (TRC_flag_T)TRC_PSMREGRESS_FLAG,
 str_NULL,
 000
},
#endif

#if defined(TRC_GROUP_PSMSYS)
{&TRC_PSMSYS_flag,
 (TRC_flag_T)TRC_PSMSYS_FLAG,
 str_NULL,
 000
},
#endif

#if defined(TRC_GROUP_REBOOT)
{&TRC_REBOOT_flag,
 (TRC_flag_T)TRC_REBOOT_FLAG,
 str_NULL,
 000
},
#endif

#if defined(TRC_GROUP_REDIR)
{&TRC_REDIR_flag,
 (TRC_flag_T)TRC_REDIR_FLAG,
 str_NULL,
 000
},
#endif

#if defined(TRC_GROUP_REMOVE)
{&TRC_REMOVE_flag,
 (TRC_flag_T)TRC_REMOVE_FLAG,
 str_NULL,
 000
},
#endif

#if defined(TRC_GROUP_RMCUI)
{&TRC_RMCUI_flag,
 (TRC_flag_T)TRC_RMCUI_FLAG,
 str_NULL,
 000
},
#endif

#if defined(TRC_GROUP_RMIOTEST)
{&TRC_RMIOTEST_flag,
 (TRC_flag_T)TRC_RMIOTEST_FLAG,
 str_NULL,
 000
},
#endif

#if defined(TRC_GROUP_RMIRADMIN)
{&TRC_RMIRADMIN_flag,
 (TRC_flag_T)TRC_RMIRADMIN_FLAG,
 str_NULL,
 000
},
#endif

#if defined(TRC_GROUP_RMIRCONFIG)
{&TRC_RMIRCONFIG_flag,
 (TRC_flag_T)TRC_RMIRCONFIG_FLAG,
 str_NULL,
 000
},
#endif

#if defined(TRC_GROUP_RMIRDB)
{&TRC_RMIRDB_flag,
 (TRC_flag_T)TRC_RMIRDB_FLAG,
 str_NULL,
 000
},
#endif

#if defined(TRC_GROUP_RMIRFAILO)
{&TRC_RMIRFAILO_flag,
 (TRC_flag_T)TRC_RMIRFAILO_FLAG,
 str_NULL,
 000
},
#endif

#if defined(TRC_GROUP_RMIRMIR)
{&TRC_RMIRMIR_flag,
 (TRC_flag_T)TRC_RMIRMIR_FLAG,
 str_NULL,
 000
},
#endif

#if defined(TRC_GROUP_RMIRSYS)
{&TRC_RMIRSYS_flag,
 (TRC_flag_T)TRC_RMIRSYS_FLAG,
 str_NULL,
 000
},
#endif

#if defined(TRC_GROUP_RMIRTRANS)
{&TRC_RMIRTRANS_flag,
 (TRC_flag_T)TRC_RMIRTRANS_FLAG,
 str_NULL,
 000
},
#endif

#if defined(TRC_GROUP_RMIRUSER)
{&TRC_RMIRUSER_flag,
 (TRC_flag_T)TRC_RMIRUSER_FLAG,
 str_NULL,
 000
},
#endif

#if defined(TRC_GROUP_RMIRUTIL)
{&TRC_RMIRUTIL_flag,
 (TRC_flag_T)TRC_RMIRUTIL_FLAG,
 str_NULL,
 000
},
#endif

#if defined(TRC_GROUP_RMTAGENT)
{&TRC_RMTAGENT_flag,
 (TRC_flag_T)TRC_RMTAGENT_FLAG,
 str_NULL,
 000
},
#endif

#if defined(TRC_GROUP_RMTBROKER)
{&TRC_RMTBROKER_flag,
 (TRC_flag_T)TRC_RMTBROKER_FLAG,
 str_NULL,
 000
},
#endif

#if defined(TRC_GROUP_RMTGUI)
{&TRC_RMTGUI_flag,
 (TRC_flag_T)TRC_RMTGUI_FLAG,
 str_NULL,
 000
},
#endif

#if defined(TRC_GROUP_RMTTEST)
{&TRC_RMTTEST_flag,
 (TRC_flag_T)TRC_RMTTEST_FLAG,
 str_NULL,
 000
},
#endif

#if defined(TRC_GROUP_SERIAL)
{&TRC_SERIALDRVR_flag,
 (TRC_flag_T)TRC_SERIALDRVR_FLAG,
 str_NULL,
 000
},
#endif

#if defined(TRC_GROUP_SETDRVR)
{&TRC_SETDRVR_flag,
 (TRC_flag_T)TRC_SETDRVR_FLAG,
 str_NULL,
 000
},
#endif

#if defined(TRC_GROUP_SETIMG)
{&TRC_SETIMG_flag,
 (TRC_flag_T)TRC_SETIMG_FLAG,
 str_NULL,
 000
},
#endif

#if defined(TRC_GROUP_SETMIR)
{&TRC_SETMIR_flag,
 (TRC_flag_T)TRC_SETMIR_FLAG,
 str_NULL,
 000
},
#endif

#if defined(TRC_GROUP_SIMPUTIL)
{&TRC_SIMPUTIL_flag,
 (TRC_flag_T)TRC_SIMPUTIL_FLAG,
 str_NULL,
 000
},
#endif

#if defined(TRC_GROUP_SNAPADMIN)
{&TRC_SNAPADMIN_flag,
 (TRC_flag_T)TRC_SNAPADMIN_FLAG,
 str_NULL,
 000
},
#endif

#if defined(TRC_GROUP_SNAPDRVR)
{&TRC_SNAPDRVR_flag,
 (TRC_flag_T)TRC_SNAPDRVR_FLAG,
 str_NULL,
 000
},
#endif

#if defined(TRC_GROUP_SNAPK10ADM)
{&TRC_SNAPK10ADM_flag,
 (TRC_flag_T)TRC_SNAPK10ADM_FLAG,
 str_NULL,
 000
};
#endif

#if defined(TRC_GROUP_SPID)
{&TRC_SPID_flag,
 (TRC_flag_T)TRC_SPID_FLAG,
 str_NULL,
 000
},
#endif

#if defined(TRC_GROUP_SVCCMI)
{&TRC_SVCCMI_flag,
 (TRC_flag_T)TRC_SVCCMI_FLAG,
 str_NULL,
 000
},
#endif

#if defined(TRC_GROUP_SVCCMITEST)
{&TRC_SVCCMITEST_flag,
 (TRC_flag_T)TRC_SVCCMITEST_FLAG,
 str_NULL,
 000
},
#endif

#if defined(TRC_GROUP_SVCCMITF)
{&TRC_SVCCMITF_flag,
 (TRC_flag_T)TRC_SVCCMITF_FLAG,
 str_NULL,
 000
},
#endif

#if defined(TRC_GROUP_SVCNDU)
{&TRC_SVCNDU_flag,
 (TRC_flag_T)TRC_SVCNDU_FLAG,
 str_NULL,
 000
},
#endif

#if defined(TRC_GROUP_SVCSCD)
{&TRC_SVCSCD_flag,
 (TRC_flag_T)TRC_SVCSCD_FLAG,
 str_NULL,
 000
},
#endif

#if defined(TRC_GROUP_SVCSCDTEST)
{&TRC_SVCSCDTEST_flag,
 (TRC_flag_T)TRC_SVCSCDTEST_FLAG,
 str_NULL,
 000
},
#endif

#if defined(TRC_GROUP_SVCSCDTF)
{&TRC_SVCSCDTF_flag,
 (TRC_flag_T)TRC_SVCSCDTF_FLAG,
 str_NULL,
 000
},
#endif

#if defined(TRC_GROUP_SYM8751)
{&TRC_SYM8751_flag,
 (TRC_flag_T)TRC_SYM8751_FLAG,
 str_NULL,
 000
},
#endif

#if defined(TRC_GROUP_TCD)
{&TRC_TCD_flag,
 (TRC_flag_T)TRC_TCD_FLAG,
 str_NULL,
 000
},
#endif

#if defined(TRC_GROUP_TDD)
{&TRC_TDD_flag,
 (TRC_flag_T)TRC_TDD_FLAG,
 str_NULL,
 000
},
#endif

#if defined(TRC_GROUP_TESTER)
{&TRC_TESTER_flag,
 (TRC_flag_T)TRC_TESTER_FLAG,
 str_NULL,
 000
},
#endif

#if defined(TRC_GROUP_TESTLDDRVR2)
{&TRC_TESTLDDRVR2_flag,
 (TRC_flag_T)TRC_TESTLDDRVR2_FLAG,
 str_NULL,
 000
},
#endif

#if defined(TRC_GROUP_TESTLDRVR)
{&TRC_TESTLDRVR_flag,
 (TRC_flag_T)TRC_TESTLDRVR_FLAG,
 str_NULL,
 000
},
#endif

#if defined(TRC_GROUP_TESTLD)
{&TRC_TESTTLD_flag,
 (TRC_flag_T)TRC_TESTTLD_FLAG,
 str_NULL,
 000
},
#endif

#if defined(TRC_GROUP_FLARE)
{&TRC_TRC_flag,
 (TRC_flag_T)TRC_TRC_FLAG,
 str_TRC_TRC,
 000
},
#endif

#if defined(TRC_GROUP_UNBIND)
{&TRC_UNBIND_flag,
 (TRC_flag_T)TRC_UNBIND_FLAG,
 str_NULL,
 000
},
#endif

#if defined(TRC_GROUP_UPTEST)
{&TRC_UPTEST_flag,
 (TRC_flag_T)TRC_UPTEST_FLAG,
 str_NULL,
 000
},
#endif

#if defined(TRC_GROUP_UTESTAPPA)
{&TRC_UTESTAPPA_flag,
 (TRC_flag_T)TRC_UTESTAPPA_FLAG,
 str_NULL,
 000
},
#endif

#if defined(TRC_GROUP_UTESTAPPB)
{&TRC_UTESTAPPB_flag,
 (TRC_flag_T)TRC_UTESTAPPB_FLAG,
 str_NULL,
 000
},
#endif

#if defined(TRC_GROUP_TMOS)
{&TRC_TMOS_flag,
 (TRC_flag_T)TRC_TMOS_FLAG,
 str_NULL,
 000
},
#endif

#if defined(TRC_GROUP_KDBMEXP)
{&TRC_KDBMEXP_flag,
 (TRC_flag_T)TRC_KDBMEXP_FLAG,
 str_NULL,
 000
},
#endif

#if defined(TRC_GROUP_KDBMEXT)
{&TRC_KDBMEXT_flag,
 (TRC_flag_T)TRC_KDBMEXT_FLAG,
 str_NULL,
 000
},
#endif
{               /* end of list */
(TRC_flag_T*)NULL,
(TRC_flag_T)0,
(char*)NULL,
(TRC_bit_name_T*)NULL
}
}; /* TRC_comps[] */

typedef struct TRC_control_hdr_struct
{
  char        magic[8];
  char       *elements;
  char      **elements_end;
  char       *strings;
  char      **strings_end;
  char       *bit_tables;
  char      **bit_tables_end;
  char      **trailer;
} TRC_control_hdr_T;

#define TRC_MAGIC "trc*tbl"

#if defined(TRC_GEN_CONTROL)
#pragma data_seg ("TRC_TABLE$1header_end")
static char elements_[4] = {'e', 'l', 'm', '\000'};

#pragma data_seg ("TRC_TABLE$2elements")
#pragma data_seg ("TRC_TABLE$3element_end")
static char *elements_end_ = (char *)0;
static char strings_[4] = {'s', 't', 'r', '\000'};

#pragma data_seg ("TRC_TABLE$4strings")
#pragma data_seg ("TRC_TABLE$5strings_end")
static char *strings_end_ = (char *)0;
static char bit_tables_[4] = {'b', 't', 't', '\000'};

#pragma data_seg ("TRC_TABLE$6bit_tables")
#pragma data_seg ("TRC_TABLE$7bit_tables_end")
static char * bit_tables_end_ = (char *)0;

#pragma data_seg ("TRC_TABLE$ztrailer")
static char* trailer_ = (char *)0;

#pragma data_seg ("TRC_TABLE$0header")
#ifndef ALAMOSA_WINDOWS_ENV
static TRC_control_hdr_T CSX_MAYBE_UNUSED TRC_control_hdr =
#else
extern TRC_control_hdr_T TRC_control_hdr =
#endif /* ALAMOSA_WINDOWS_ENV - ODDCASE - compiler quirks */
{
  TRC_MAGIC,
   elements_,
  &elements_end_,
   strings_,
  &strings_end_,
   bit_tables_,
  &bit_tables_end_,
  &trailer_
};
#endif /* defined(TRC_GEN_CONTROL) */
#endif /* defined(GEN_DATA) */
#pragma data_seg (".data")
#endif /* !defined(TRC_ENVIRON2_H) */
/* last line in file */