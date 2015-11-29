#ifndef CA_STATES_H
#define CA_STATES_H 0x00000001  /* from common dir */
#define FLARE__STAR__H

/**************************************************************
 *  Copyright (C)  EMC Corporation 1993-2007
 *  All rights reserved.
 *  Licensed material - property of EMC Corporation.
 **************************************************************/


/*********************************************************
 *  $Id: ca_states.h,v 1.2 1999/06/29 23:09:53 fladm Exp $
 *********************************************************
 *
 *  Description:
 *      This file contains constants used for various
 *	Cache states.
 *
 *  History:
 *       3-Feb-93 Created.                    Steve Morley
 *	16-Sep-93 Added WAITING_FOR_CORRUPT_CRC - RER
 *	22-Sep-93 Added comments		RER
 *	08-Fed-94 Added CA_PING_PONG		RER
 *	14-Apr-94 added CA_RAID3_DEGRADED	RER
 *	08-AUg-94 added CA_CACHE_RECOVERING	RER
 *	16-Sep-94 added CA_COMMITTED_SUICIDE	RER
 *	06-Nov-95 added CA_PEER_FR3_FROZEN		RER
 *	23-Oct-97 modified for virtual driver design.  
 *            File renamed from ca_states.h. bkhung
 *
 *  $Log: ca_states.h,v $
 *  Revision 1.2  1999/06/29 23:09:53  fladm
 *  User: gpeterson
 *  Reason: EPR_1601
 *  added local & peer safe loaded states states, for use in
 *  the state machine long (ported from K5).
 *
 *  Revision 1.1  1999/01/05 22:23:09  fladm
 *  User: dibb
 *  Reason: initial
 *  Initial tree population
 *
 *  Revision 1.7  1998/10/20 16:52:42  bkhung
 *   Modified Shutdown/Release code.
 *
 *  Revision 1.6  1998/07/09 14:02:16  skasera
 *   Added a #define CA_NUM_CACHE_STATES for total number of cache states
 *
 *  Revision 1.5  1998/06/12 18:42:14  bkhung
 *   Modified to handle BUSY status from the Clean Dirty Driver and to refrain from sending redundant clean/dirty requests.
 *
 *  Revision 1.4  1998/04/27 17:38:03  bkhung
 *   Added new define CA_LOWER_DRIVER_FROZEN.
 *
 *  Revision 1.3  1998/04/09 15:30:15  bkhung
 *   Replaced CA_PING_PONG with CA_PEER_HARDWARE_OK.
 *
 *  Revision 1.2  1998/03/12 15:38:42  bkhung
 *   Ported fix for K5 bug 1268/1269.  Changed CA_PEER_FR3_FROZEN and CA_ACK_PEER_FR3_FROZEN to CA_PEER_CHANNEL_OPEN and CA_PEER_CHANNEL_CLOSED.
 *
 *  Revision 1.1  1998/01/19 22:15:24  bkhung
 *  Initial revision
 *
 *  Revision 1.1  1997/12/19 17:12:27  bkhung
 *  Initial revision
 *
 *	07-Jul-03 Added WAITING_FOR_BITMAP1_LOAD minor state for Vault Load - keisling
 *
 *********************************************************/




/*
 * LOCAL TEXT unique_file_id[] = "$Header: /fdescm/project/CVS/flare/src/include/ca_states.h,v 1.2 1999/06/29 23:09:53 fladm Exp $";
 */



/*********************
 *   INCLUDE FILES   *
 *********************/

/****************
 *   LITERALS   *
 ****************/


	/*
	 * The Cache cache is initing.  This is the initial value of our
	 * ca_cache_state when powering up.
	 */

#define CA_CACHE_INITING              0

	/*
	 * When the peer is powering (determined by a peer initing event) 
	 * the running board enters the syncing state where the two boards
	 * are attempting the sync the cache ram images.  The board
	 * remains syncing until the peer either dies or transitions to
	 * to another state (ie ENABLING, DISABLING or DISABLED).
	 */

#define CA_CACHE_SYNCING              1

	/*
	 * Before the cache is ENABLED, each board enters the ENABLING state.
	 * Enabling is simply a handshake between the two Caches indicating that
	 * each is ready to enable.
	 */

#define CA_CACHE_ENABLING             2


	/*
	 * The cache is ENABLED while all necessary caching components are
	 * enabled.  These components are, the peer is enabled, the raid3
	 * backup disks are enabled, the bbu is charged, the fans and VSCs
	 * are not faulty.  If one or more of these components should fail,
	 * the cache is disabled.
	 */

#define CA_CACHE_ENABLED              3



	/*
	 * When a required component fails, the cache image is backed up on
	 * our raid3 database disks (referred to as the safe).  Before the
	 * writes to the disk can begin, all cache ram modifications must be
	 * stopped (due to parity encoding).  This is referred to as quiescing.
	 * Quiescing state is when all CAQEs are being stopped.  Once all
	 * active CAQEs are stopped, the cache is said to be frozen.
	 */

#define CA_CACHE_QUIESCING            4


	/*
	 * All CAQEs are frozen.  But before backing up the cache image, we
	 * must wait for all CMI traffic from the other board to stop (for
	 * the same reason as above).  When the peer responds that it is also
	 * frozen (or dead), The cache image backup can take place.
	 */

#define CA_CACHE_FROZEN               5


	/*
	 * One of the boards is sumping the cache to the backup media.
	 */

#define CA_CACHE_DUMPING              6


	/*
	 * The cache is disabling while there are components failures and
	 * the cache is dirty.  When the cache is cleaned, the cache is
	 * said to be disabled.  should all the required components become
	 * enabled, we attempt to enabled the cache.
	 */

#define CA_CACHE_DISABLING            7


	/*
	 * The cache is disabled while there are components failures (and
	 * the cache is clean). Should all the required components become
	 * enabled, we attempt to enabled the cache.
	 */

#define CA_CACHE_DISABLED             8


	/*
	 * The cache is recovering if we are caching on a single board
	 * (non-mirrored) and the CM tells us that cache recovery is needed.
	 * The RECOVERING state is similar to the INITING state, in that the
	 * Front End is not yet turned on, and units are not yet assigned.
	 * After a successful recovery, we will transition to DUMPING state.
	 */

#define CA_CACHE_RECOVERING           9

   /* Number of cache states defined. If new cache state(s) is(are) added,
    * CA_NUM_CACHE_STATES should be updated appropriately.
    */
#define CA_NUM_CACHE_STATES           10

/*
 * The following are all the minor states for CAQEs
 */

#define WAITING_FOR_FORCED_FLUSH           1
#define WAITING_FOR_WATERMARK_FLUSH        2
#define WAITING_FOR_NONFORCED_FLUSH        3
#define WAITING_FOR_QUIESCE                4
#define WAITING_FOR_BITMAP_UPDATE          5
#define WAITING_FOR_BACKFILL               6
#define WAITING_FOR_BACKFILL_AND_CMI       7        /* Used only in fdb */
#define WAITING_FOR_BACKFILL_OK            8        /* Used only in fdb */
#define WAITING_FOR_BACKFILL_ERROR         9        /* Used only in fdb */
#define WAITING_FOR_BACKFILL_ABORTED      10        /* Used only in fdb */
#define WAITING_FOR_NORMAL_IO             11
#define WAITING_FOR_BUFFER_READ           12        /* Used only in fdb */
#define WAITING_FOR_SYNCRONIZE            13        /* Used only in fdb */
#define WAITING_FOR_ABORT                 14        /* Used only in fdb */
#define WAITING_FOR_CHAIN                 15
#define WAITING_FOR_PAGES                 16
#define WAITING_FOR_READ_PAGES            17


#define WAITING_FOR_MCB                   19        /* Used only in fdb */
#define WAITING_FOR_SG                    20
#define WAITING_FOR_READ_SG               21

#define WAITING_FOR_CCM                   22        /* Used only in fdb */
#define WAITING_FOR_CMI                   23
#define WAITING_FOR_FED                   24
#define WAITING_FOR_FED_AND_CMI           25        /* Used only in fdb */
#define WAITING_FOR_FED_DISABLING_FLUSH   26        /* Used only in fdb */
#define WAITING_FOR_CMI_DISABLING_FLUSH   27        /* Used only in fdb */
#define WAITING_FOR_STATUS                28        /* Used only in fdb */
#define WAITING_FOR_DIRTY_REQ             29
#define WAITING_FOR_FUA_WRITE             30
#define WAITING_FOR_BITMAP                31        /* Used only in fdb */
#define WAITING_FOR_CORRUPT_CRC           32        /* Used only in fdb */
#define WAITING_FOR_UNBIND_UNIT           33
#define WAITING_FOR_ASSIGN_UNIT           34
#define WAITING_FOR_RELEASE_UNIT          35        /* Used only in fdb */
#define WAITING_FOR_CHANGE_UNIT_CACHE     36
#define WAITING_FOR_CACHE_SHUTDOWN_RELEASE 37
#define WAITING_FOR_SHUTDOWN_RELEASE_UNIT 38
#define WAITING_FOR_CLEAN_REQ             39
#define WAITING_FOR_FED_XFER              40
#define WAITING_FOR_LOWER_DRIVER          41
#define WAITING_FOR_RESUME_FROM_ED        42
#define WAITING_FOR_SYNC                  43
#define WAITING_FOR_BITMAP1_LOAD          44
#define WAITING_FOR_DEADLOCK_AVOIDANCE_WRITE 45
#define WAITING_FOR_DEADLOCK_AVOIDANCE_FLUSH 46
#define WAITING_FOR_DEADLOCK_AVOIDANCE_READ  47
#define WAITING_FOR_ZERO                  48
#define WAITING_FOR_RD_PIPELINE              49
#define CA_IMG_XFER_META                  50
#define CA_IMG_XFER_BITMAP                51
#define CA_IMG_XFER_HEADER                52
#define CA_IMG_XFER_DATA                  53

/*
 * Below are all the events defined in the cache state machine's protocol.
 */

	/* We initialize the various states (ca_raid3_state, ca_hw_state,
	   ca_cache_state, ca_peer_state) to CA_UNKNOWN_STATE when we are
	   initializing, before we know what the real states should be.
	 */

#define CA_UNKNOWN_STATE              0

	/* CA_RAID3_DISABLED indicates that we have received a MSG_SAFE_DISKS_GONE
	   message from the ATM, indicating that the raid 3 safe is not usable.
	 */

#define CA_RAID3_DISABLED             1

	/* CA_RAID3_ENABLED indicates that we have received a MSG_SAFE_DISKS_OK
	   message from the ATM, indicating that the raid 3 safe disks can be used
	   for dumping the cache.
	 */

/* Logically CA_RAID3_DEGRADED would go here, but I don't want to renumber! */

#define CA_RAID3_ENABLED              2

	/* CA_SOFT_SHUTDOWN_IMMINENT indicates that we have received a CA_SEND_DIAGNOSTIC
	   message from the AEP.  We've been asked to kill ourselves, but first we need
	   to shut down the cache.
	 */

#define CA_SOFT_SHUTDOWN_IMMINENT     3

	/* CA_SHUTDOWN_IMMINENT indicates that we have received a MSG_HARDWARE_BAD_SUICIDE
	   message from the ATM.  There is a hardware problem which means we cannot continue
	   for long, so try to shut down the cache.
	 */

#define CA_SHUTDOWN_IMMINENT          4

	/* CA_HARDWARE_FAULT indicates that we have received a MSG_HARDWARE_BAD_KEEP_RUNNING
	   message from the ATM.
	 */

#define CA_HARDWARE_FAULT             5

	/* CA_HARDWARE_OK means that we have received a MSG_HARDWARE_OK message from the
	   ATM, indicating that the hardware is functional and will support caching.
	 */

#define CA_HARDWARE_OK                6

	/* The CA_FROZEN event is generated when we are quiescing or synching and the
	   caqe_running_count has gone to zero.
	 */

#define CA_FROZEN                     7

	/* The CA_CLEAN event is generated when all pages in the cache
	   are clean.
	 */

#define CA_CLEAN                      8

	/* CA_PEER_SYNCING means that the peer is waiting for us to read the SAFE.
	 */

#define CA_PEER_SYNCING               9

	/* The CA_PEER_INITING event is generated when the peer is initing
	   and wishes to read the SAFE.
	 */

#define CA_PEER_INITING               10

	/* The CA_PEER_TRANSIENT event is generated when we get the first NAK
	   status from a CMI write, or when we get a CA_MSG_CM_PEER_REMOVED message
	   from the CM.  The peer appears to be dead, but we first we need to
	   clear out all CMI messages from the dead peer.
	 */

#define CA_PEER_TRANSIENT             11

	/* The CA_PEER_DEAD event is generated when we get a CLOSE_ACK status
	   from CMI, indicating that we have cleared out all CMI messages from the
	   dead peer and have closed the CMI channel.
	 */

#define CA_PEER_DEAD                  12

	/* The following events are generated when the peer has informed us
	   that it has entered the corresponding cache state
	 */

#define CA_PEER_ENABLING              13
#define CA_PEER_ENABLED               14
#define CA_PEER_QUIESCING             15
#define CA_PEER_FROZEN                16
#define CA_PEER_DUMPING               17
#define CA_PEER_DISABLING             18
#define CA_PEER_DISABLED              19

	/* The CA_PEER_BACKUP_OK event is generated when the peer
	   has informed us that it has completed a SAFE dump
	   successfully
	 */

#define CA_PEER_BACKUP_OK             20

	/* The CA_PEER_BACKUP_ERROR event is generated when the
	   peer has informed us that a SAFE dump has failed.
	 */

#define CA_PEER_BACKUP_ERROR          21

	/* The CA_PEER_BACKUP_NOT_NEEDED event is generated when
	   the peer has informed us that a SAFE dump is not needed.
	   IT IS PRESENTLY NOT USED.
	 */

#define CA_PEER_BACKUP_NOT_NEEDED     22

	/* CA_BACKUP_OK means that we received a MSG_COMPLETED_SAFE_DUMP 
	   message from the ATM, and the dump was completed successfully.
	 */

#define CA_BACKUP_OK                  23

	/* CA_BACKUP_ERROR means that we received a MSG_COMPLETED_SAFE_DUMP 
	   message from the ATM, and the dump failed.
	 */

#define CA_BACKUP_ERROR               24

	/* The CA_STOP_CACHE event is generated in order to deallocate the 
	   cache table in preparation for changing cache parameters.
	 */

#define CA_STOP_CACHE		          25

	/* Peer notification that it's cache hardware is ok.  */
#define CA_PEER_HARDWARE_OK           26

	/* The CA_INIT_NEW_CACHE event is generated after we change cache
	   parameters.
	 */

#define CA_INIT_NEW_CACHE             27

	/* The CA_NEW_CACHE_SESSION event is generated when we get a
	   CA_MSG_CM_ENABLING_OK message from the CM. 
	 */

#define CA_NEW_CACHE_SESSION          28

	/* The CA_ENABLE_CACHE event is generated when we get a
	   CA_MSG_CM_ENABLE_CACHE message from the CM.
	 */

#define CA_ENABLE_CACHE               29

	/* The CA_DISABLE_CACHE event is generated when we get a
	   CA_MSG_CM_DISABLE_CACHE message from the CM.
	 */

#define CA_DISABLE_CACHE              30

	/* The CA_DIRTY_LUN_ASSIGNED event is generated when a unit is
	   assigned with dirty pages.  If we are disabled when this
	   happens, we need to transition to disabling.
	 */

#define CA_DIRTY_LUN_ASSIGNED	       31

	/* The CA_NOT_ENOUGH_CACHE_PAGES event is generated when there
	   are not enough cache pages to enable caching.
	 */

#define CA_NOT_ENOUGH_CACHE_PAGES     32

	/* The CA_ENOUGH_CACHE_PAGES event is generated when there
	   are now enough cache pages to enable caching.
	 */

#define CA_ENOUGH_CACHE_PAGES	       33

	/* The CA_DUMP_NEEDED event is similar to CA_DIRTY_LUN_ASSIGNED
	   Calling it ensures that we dump the safe before the peer tries
	   reading it.
	 */

#define CA_DUMP_NEEDED		       34

	/* CA_PEER_SAFE_STATUS means that the peer has just loaded the safe,
     * or has just given US permission to load the safe.
	 * The status of the peer's load attempt is in cm_PeerSafeState.
	 */

#define CA_PEER_SAFE_STATUS	       35

	/* CA_SAFE_LOADED means that the safe has been loaded after we got
	 * an 'enable cache' request when we were not using the cache.
	 * The status of that load attempt is in cm_SafeState.
	 */

#define CA_SAFE_LOADED		       36

	/* CA_RAID3_DEGRADED means that one of the safe drives is down
	 * or rebuilding.  The safe is still valid, but we can't cache.
	 */

#define CA_RAID3_DEGRADED	       37

	/* CA_COMMITTED_SUICIDE means that the Cache sent a MSG_COMMIT_SUICIDE
	 * or MSG_WAIT_FOR_HARDWARE_OK message to the ATM and is waiting for
	 * MSG_ENV_STATE_RETURNED_TO_OKAY.
	 */

#define CA_COMMITTED_SUICIDE	       38

/* Informatory State change indicating peers view of its CMB channel */
#define CA_PEER_CHANNEL_OPEN        39
#define CA_PEER_CHANNEL_CLOSED      40


/* Safe Load Statues: These should match the list in atm.h */

#define CA_SAFE_LOAD_DATA_OK                  41
#define CA_SAFE_LOAD_FAILED                   42
#define CA_SAFE_LOAD_INCONSISTENT             43
#define CA_SAFE_LOAD_FAILED_BITMAP_OK         44
#define CA_SAFE_LOAD_FAILED_NOT_ENOUGH_MEMORY 45
#define CA_SAFE_DISKS_SCRAMBLED               46

/* Peer Safe Load Statues: These should match the list in atm.h */

#define CA_PEER_SAFE_NOT_YET_LOADED                47
#define CA_PEER_SAFE_LOAD_DATA_OK                  48
#define CA_PEER_SAFE_LOAD_FAILED                   49
#define CA_PEER_SAFE_LOAD_INCONSISTENT             50
#define CA_PEER_SAFE_LOAD_FAILED_BITMAP_OK         51
#define CA_PEER_SAFE_LOAD_FAILED_NOT_ENOUGH_MEMORY 52
#define CA_PEER_SAFE_DISKS_SCRAMBLED               53

/*
 * CA_LOWER_DRIVER_FROZEN event is generated when the lower driver
 * has informed the cache that it is frozen.
 */
#define  CA_LOWER_DRIVER_FROZEN                   54

/* An 'initing' state, to go into the runtime log. */
#define CA_INITING                                55

/* Master Peer has died while disabling. This SP may need to snapshot bitmap */
#define CA_FORCE_BITMAP_UPDATE                    56

/* Support messages for 'clean' vault dumps */
#define CA_CLEAN_BACKUP_OK                        57
#define CA_PEER_CLEAN_BACKUP_OK                   58

/* Peer has been frozen but with dirty pages, can not do 'clean' vault dump */
#define CA_PEER_FROZEN_DIRTY                      59

// For rev number support
#define CA_REV_REQ                                60
#define CA_REV_NUM                                61

/* all cache dirty units are cleared */
#define CA_CACHE_DIRTY_NONE                       62

/* Tell the peer that cache dirty units are cleared */
#define CA_PEER_CACHE_DIRTY_CLEARED               63

/* Tell the state machine that cache state timedout */
#define CA_STATE_TIMEDOUT                         64

/* Tell the state machine that waiting for peer response timedout */
#define CA_WAIT_FOR_PEER_RESP_TIMEDOUT            65

/* Tell the state machine the vault dump was aborted */
#define CA_BACKUP_ABORTED                         66

/* Message from peer state machine that backup was aborted */
#define CA_PEER_BACKUP_ABORTED                    67

// For CARP (Cache Recovery From Peer) support.
// 1) Sanity check messages via Cmi_L1CacheEvent_Conduit.
//    CA_WC_SIZE_REQUEST: the surviving  SP to the recovering SP.
//    CA_WC_SIZE_RETURN:  the recovering SP to the surviving  SP.
// 2) Transfer completion handshakes via Cmi_L1CacheEvent_Conduit.
//    CA_VAULT_IMG_XFERED:     the surviving  SP to the recovering SP.
//    CA_VAULT_IMG_XFERED_ACK: the recovering SP to the surviving  SP.
// 3) CA_VAULT_IMG_XFER_ABORTED: event to cache state machine when fails.
//    CA_VAULT_IMG_XFER_COMM_ERR: event to CSM log on CMI error.
//    CA_VAULT_IMG_XFER_SW_REV_ERR: event to CSM log on version check failure.
//    CA_VAULT_IMG_XFER_WC_SIZE_ERR: event to CSM log on WC size check failure.
//    CA_VAULT_IMG_XFER_ENABLED: event notifying when CARP ENABLED.
//    CA_VAULT_IMG_XFER_DISABLED: event notifying when CARP DISABLED.
#define CA_WC_SIZE_REQUEST                        68
#define CA_WC_SIZE_RETURN                         69
#define CA_VAULT_IMG_XFERED                       70
#define CA_VAULT_IMG_XFERED_ACK                   71
#define CA_VAULT_IMG_XFER_ABORTED                  72
#define CA_VAULT_IMG_XFER_COMM_ERR                73
#define CA_VAULT_IMG_XFER_SW_REV_ERR              74
#define CA_VAULT_IMG_XFER_WC_SIZE_ERR             75
#define CA_VAULT_IMG_XFER_ENABLED                 76
#define CA_VAULT_IMG_XFER_DISABLED                77

/* Events for Write Cache Availability */
#define CA_ENABLED_WCA                            78
#define CA_DISABLED_WCA                           79

// Indicates we tried to do a vault XFER without our copy
// of the peer data table which is needed for Local Address
// Translation
#define CA_VAULT_IMG_XFER_NO_TBL_ERR              80

/* CARP specific revision request and response
 */
#define CA_REV_REQ_CARP                           81
#define CA_REV_NUM_CARP                           82

/* This message acknowledges over the CM conduit
 * that we saw the peer's vault dump finish.
 * It now has permission to give the sender permission 
 * to load said vault.
 */
#define CA_PEER_DUMP_COMPLETED_ACK                83

/* Rebuild related message.
*/
#define CA_REBUILD_FAILED                         84
#define CA_REBUILD_SUCCEED                        85

#define CA_PEER_REBUILD_FAILED                    86
#define CA_PEER_REBUILD_SUCCEED                   87

/* Events for communicating that SP not allowed to
 * load vault image at that point in time.
 */
#define CA_SAFE_LOAD_DENIED                       88
#define CA_SAFE_LOAD_DENIED_ACK                   89

#define CA_SAFE_NOT_YET_LOADED                    90

/* Verbose cache rebuild events */
#define CA_REBUILD_FAIL_INV_PAGE_NUMBER         91
#define CA_REBUILD_FAIL_INV_LUN                 92
#define CA_REBUILD_FAIL_INV_LDA                 93
#define CA_REBUILD_FAIL_INV_BITS                94
#define CA_REBUILD_FAIL_INV_OWNER               95

/* This is just a place holder to help get some debug checks
 * in the code.  Please always increment this event and 
 * keep it as the last cache event when adding new events.
 * There is currently no string in event_string for this event.
 */
#define CA_MAX_EVENT                              96

/*
 * CA_STATE_MACHINE is used in the request id field
 * of the cmi msg indicating that the CMB is owned by the
 * cache state machine.  Thus, the CMB should be given
 * back to the cache state machine when ack'd
 * (see ca_peer_ack() in ca_state_machine.c).
 */

#define CA_STATE_MACHINE               1

/*
 * CA_BITMAP_MACHINE is used in the request id field
 * of the cmi msg indicating that the CMB is owned by the
 * bitmap code.
 */

#define CA_BITMAP_MACHINE              2


/*
 * CA_GET_PAGES_MACHINE is used in the request id field 
 * of the cmi msg indicating that the CMB is owned by the
 * ca_get_pages code.
 */

#define CA_GET_PAGES_MACHINE           3


/*
 * CA_TRACE_MACHINE is used for gathering traces of
 * host traffic.  We write the lun,lda and len into
 * a buffer and flush it to a dedicated disk when it
 * fills.
 */

#define CA_TRACE_MACHINE               4

/*
 * CA_XLATION_MACHINE is used in request id field
 * of the cmi msg indicating that the CMB is owned by
 * ca_send_out_translation_information code. 
 */

#define CA_XLATION_MACHINE             5

#define CA_NUM_CMB_OWNERS              5

#endif /* ndef CA_STATES_H */
