/* $CCRev: ./array_sw/catmerge/interface/SyncTransaction.h@@/main/base0/2 */
/************************************************************************
 *
 *  Copyright (c) 2004,2005,2006,2007,2010 EMC Corporation.
 *  All rights reserved.
 *
 *  ALL RIGHTS ARE RESERVED BY EMC CORPORATION.  ACCESS TO THIS
 *  SOURCE CODE IS STRICTLY RESTRICTED UNDER CONTRACT. THIS CODE IS TO
 *  BE KEPT STRICTLY CONFIDENTIAL.
 *
 *  UNAUTHORIZED MODIFICATIONS OF THIS FILE WILL VOID YOUR SUPPORT
 *  CONTRACT WITH EMC CORPORATION.  IF SUCH MODIFICATIONS ARE FOR
 *  THE PURPOSE OF CIRCUMVENTING LICENSING LIMITATIONS, LEGAL ACTION
 *  MAY RESULT.
 *
 ************************************************************************
 *
 *  2004-12-03 12:27pm  fabx-r2590       vphan     .../v0.1.16/1
 *  2004-08-10 03:53pm  fabx-r1551       vphan     .../v0.1.12/1
 *  2004-07-12 01:40pm  fabx-r1338       vphan     .../v0.1.11/1
 *  2004-06-15 01:58pm  fabx-r1173       scooper   .../v0.1.10/2
 *  2004-06-03 02:42pm  fabx-r1052       scooper   .../v0.1.10/1
 *
 ************************************************************************/

#ifndef _SYNCTRANS_
#define _SYNCTRANS_

//
// #defines
//

// The array of TRANSACTION_DESCRIPTIONs is terminated
// by an entry with TransactionKey set to this constant.
#define TRANSKEY_TERMINATOR     0xffffffffUL

// The following value can be returned by the TransactionValidate function
// to indicate that transaction processing can proceed only if Validate on
// the peer SP returns the same value or STATUS_SUCCESS.
#define STATUS_OK_IF_PEER_OK    ((EMCPAL_STATUS)0x00000128L)

// The following value can be returned by the TransactionValidate function
// to indicate that transaction is being used only for acquiring the global lock.
// The global lock is then released and the transaction is completed.
#define STATUS_ACQUIRING_LOCK_ONLY  ((EMCPAL_STATUS)0x00000129L)

//
// Typedefs
//

struct transaction_description;

// A reference to a TRANSACTION_ARGS structure is suppied to each of the
// user-supplied functions that are invoked during transaction processing.
// The TransactionKey, Description, ParamLen, Params, and Context are those
// supplied by the caller as arguments to DoSyncTransaction().
typedef struct {
    BOOLEAN     IsInitiatingSP;
    BOOLEAN     ContactLost;
    ULONG       TransactionNumber;
    ULONG       State;
    ULONG       PrevState;
    ULONG       PeerResponse;
    struct transaction_description * pTransDesc;

    // The transaction
    ULONG       TransactionKey;
    ULONG       ParamsLen;
    // Set by initiating SP, transferred to the !IsInitiatingSP on intent.
    ULONGLONG   IntentionInformation;
    CHAR *      Params;

    CHAR *      Description;

    // For caller use
    VOID *      Context;
} TRANSACTION_ARGS;

// A TRANSACTION_INTENTION structure is used to convey information about
// the current transaction to the peer SP.  Once it has been prepared using
// the arguments supplied to DoSyncTransaction, it will be made available
// to the TransactionPrepareIntention function (if the caller has supplied
// one) for possible modification.  Neither TransactionNumber nor
// TransactionKey should normally be changed.
typedef struct {
    ULONG       TransactionNumber;
    ULONG       TransactionKey;
    ULONG       Spare;
    ULONG       ParamsLen;
    ULONGLONG   IntentionInformation;
    CHAR *      Params;
} TRANSACTION_INTENTION;

// Optional, caller-supplied Transaction Functions:
typedef ULONG       (*TransactionGetTransNumber)    (VOID);
typedef EMCPAL_STATUS    (*TransactionValidate)          (TRANSACTION_ARGS * args);
typedef EMCPAL_STATUS    (*TransactionPersist)           (TRANSACTION_ARGS * args);
typedef VOID        (*TransactionPrepareIntention)  (TRANSACTION_ARGS * args,
                                                     TRANSACTION_INTENTION * intention);
typedef VOID        (*TransactionApplyInternal)     (TRANSACTION_ARGS * args);
typedef VOID        (*TransactionCleanup)           (TRANSACTION_ARGS * args);
typedef VOID        (*TransactionApplyExternal)     (TRANSACTION_ARGS * args);

// TRANSACTION DESCRIPTION
// The caller of DoSyncTransaction will first supply an array of
// TRANSACTION_DESCRIPTIONs, where each entry describes a specific
// transaction task.
// The description of a transaction consists of:
//      * a unique TransactionKey that identifies the specific task
//        of the transaction (e.g., command or event id);
//      * a reference to a description of the transaction task;
//      * the maximum length of the parameter string (including
//        trailing NULL) that might be supplied for this
//        specific task. Keep in mind that the original parameters
//        may be modified by the optional PrepareIntention module;
//      * references to the caller-supplied modules that will be
//        invoked by the transaction mechanism to process the
//        specific task identified by the TransactionKey.  They
//        are all optional; NULL indicates the absence of a
//        particular module.
// The array of TRANSACTION_DESCRIPTIONs must be terminated by an
// entry with TransactionKey set to TRANSKEY_TERMINATOR (0xffffffffUL).
typedef struct transaction_description {
    ULONG                           TransactionKey;
    CHAR *                          Description;
    ULONG                           MaxParamsLen;
    TransactionValidate             Validate;
    TransactionPrepareIntention     PrepareIntention;
    TransactionApplyInternal        ApplyInternal;
    TransactionPersist              Persist;
    TransactionApplyExternal        ApplyExternal;
    TransactionCleanup              Cleanup;
} TRANSACTION_DESCRIPTION;

//
// Function Prototypes
//
// SyncTransactionInitialize must be called successfully before the first
// invocation of DoSyncTransaction.  Arguments are:
//      TransDescArray  Pointer to an array of TRANSACTION_DESCRIPTION elements.
//      DomainName      A string, delimited by single quote marks, with up to 4
//                      characters.  It is used in conjuction with MailboxName
//                      to identify a communication path to the other SP that
//                      will be established by the transaction mechanism.
//      MailboxName     A pointer to a wide character string representation of
//                      the mailbox name.
//      SyncLockName    A character string used to identify the distributed SYNC
//                      lock used by the transaction mechanism (max 256 chars).
//      GetTransactionNumber
//                      Function that returns the last successful transaction
//                      number.  The transaction mechanism will increment this
//                      number for each subsequent transaction request, and make
//                      the result available to all user-supplied modules during
//                      processing of each transaction.  Can be NULL, in which
//                      case the value 0 will be used.

extern EMCPAL_STATUS SyncTransactionInitialize(TRANSACTION_DESCRIPTION *    TransDescArray,
                                          ULONG                        DomainName,
                                          csx_pchar_t                  MailboxName,
                                          ULONG                        MailboxNameLength,
                                          CHAR *                       SyncLockName,
                                          TransactionGetTransNumber    GetTransactionNumber,
                                          PVOID                        pContext);

extern VOID SyncTransactionCleanup(VOID);

// DoSyncTransaction performs the designated transaction in the context of the synchronization/
// persistence mechanism.  Arguments are:
//      TransactionKey  Identifies a specific transaction; used to locate the appropriate
//                      set of user-supplied functions in the TransDescArray supplied
//                      to SyncTransactionInitialize.
//      Params          A pointer to the parameters supplied for this invocation of the
//                      transaction.
//      ParamsLen       The length of Params.
//      Context         User-defined; may be NULL.

extern EMCPAL_STATUS DoSyncTransaction(ULONG TransactionKey,
                                  CHAR * Params,
                                  ULONG ParamsLen,
                                  VOID * Context);

extern ULONG SyncGetCurrentTransactionNumber(void);

#endif // _SYNCTRANS_

