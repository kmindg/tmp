//#include "ntddscsi.h"

typedef unsigned char U8;
typedef unsigned short U16;
typedef unsigned int U32;

typedef struct _U64
{
    U32          Low;
    U32          High;
} U64;

#define MPI_POINTER     *

typedef struct
{
	SRB_IO_CONTROL	 Sic;
	UCHAR			 Buf[8+128+1024];
} SRB_BUFFER;

/****************************************************************************/
/*  SGE IO types union  for IO SGL's                                        */
/****************************************************************************/

typedef struct _SGE_SIMPLE_UNION
{
    U32                     FlagsLength;
    union
    {
        U32                 Address32;
        U64                 Address64;
    }u;
} SGE_SIMPLE_UNION, MPI_POINTER PTR_SGE_SIMPLE_UNION,
  SGESimpleUnion_t, MPI_POINTER pSGESimpleUnion_t;

typedef struct _SGE_CHAIN_UNION
{
    U16                     Length;
    U8                      NextChainOffset;
    U8                      Flags;
    union
    {
        U32                 Address32;
        U64                 Address64;
    }u;
} SGE_CHAIN_UNION, MPI_POINTER PTR_SGE_CHAIN_UNION,
  SGEChainUnion_t, MPI_POINTER pSGEChainUnion_t;

typedef struct _SGE_IO_UNION
{
    union
    {
        SGE_SIMPLE_UNION    Simple;
        SGE_CHAIN_UNION     Chain;
    } u;
} SGE_IO_UNION, MPI_POINTER PTR_SGE_IO_UNION,
  SGEIOUnion_t, MPI_POINTER pSGEIOUnion_t;

typedef struct _MSG_IOC_INIT
{
    U8                      WhoInit;                    /* 00h */
    U8                      Reserved;                   /* 01h */
    U8                      ChainOffset;                /* 02h */
    U8                      Function;                   /* 03h */
    U8                      Flags;                      /* 04h */
    U8                      MaxDevices;                 /* 05h */
    U8                      MaxBuses;                   /* 06h */
    U8                      MsgFlags;                   /* 07h */
    U32                     MsgContext;                 /* 08h */
    U16                     ReplyFrameSize;             /* 0Ch */
    U8                      Reserved1[2];               /* 0Eh */
    U32                     HostMfaHighAddr;            /* 10h */
    U32                     SenseBufferHighAddr;        /* 14h */
    U32                     ReplyFifoHostSignalingAddr; /* 18h */
    SGE_SIMPLE_UNION        HostPageBufferSGE;          /* 1Ch */
    U16                     MsgVersion;                 /* 28h */
    U16                     HeaderVersion;              /* 2Ah */
} MSG_IOC_INIT, MPI_POINTER PTR_MSG_IOC_INIT,
  IOCInit_t, MPI_POINTER pIOCInit_t;

/* WhoInit values */
#define MPI_WHOINIT_NO_ONE                              (0x00)
#define MPI_WHOINIT_SYSTEM_BIOS                         (0x01)
#define MPI_WHOINIT_ROM_BIOS                            (0x02)
#define MPI_WHOINIT_PCI_PEER                            (0x03)
#define MPI_WHOINIT_HOST_DRIVER                         (0x04)
#define MPI_WHOINIT_MANUFACTURER                        (0x05)

/* Flags values */
#define MPI_IOCINIT_FLAGS_HOST_PAGE_BUFFER_PERSISTENT   (0x04)
#define MPI_IOCINIT_FLAGS_REPLY_FIFO_HOST_SIGNAL        (0x02)
#define MPI_IOCINIT_FLAGS_DISCARD_FW_IMAGE              (0x01)

/* MsgVersion */
#define MPI_IOCINIT_MSGVERSION_MAJOR_MASK               (0xFF00)
#define MPI_IOCINIT_MSGVERSION_MAJOR_SHIFT              (8)
#define MPI_IOCINIT_MSGVERSION_MINOR_MASK               (0x00FF)
#define MPI_IOCINIT_MSGVERSION_MINOR_SHIFT              (0)

/* HeaderVersion */
#define MPI_IOCINIT_HEADERVERSION_UNIT_MASK             (0xFF00)
#define MPI_IOCINIT_HEADERVERSION_UNIT_SHIFT            (8)
#define MPI_IOCINIT_HEADERVERSION_DEV_MASK              (0x00FF)
#define MPI_IOCINIT_HEADERVERSION_DEV_SHIFT             (0)


typedef struct _MSG_IOC_INIT_REPLY
{
    U8                      WhoInit;                    /* 00h */
    U8                      Reserved;                   /* 01h */
    U8                      MsgLength;                  /* 02h */
    U8                      Function;                   /* 03h */
    U8                      Flags;                      /* 04h */
    U8                      MaxDevices;                 /* 05h */
    U8                      MaxBuses;                   /* 06h */
    U8                      MsgFlags;                   /* 07h */
    U32                     MsgContext;                 /* 08h */
    U16                     Reserved2;                  /* 0Ch */
    U16                     IOCStatus;                  /* 0Eh */
    U32                     IOCLogInfo;                 /* 10h */
} MSG_IOC_INIT_REPLY, MPI_POINTER PTR_MSG_IOC_INIT_REPLY,
  IOCInitReply_t, MPI_POINTER pIOCInitReply_t;



/****************************************************************************/
/*  IOC Facts message                                                       */
/****************************************************************************/

typedef struct _MSG_IOC_FACTS
{
    U8                      Reserved[2];                /* 00h */
    U8                      ChainOffset;                /* 01h */
    U8                      Function;                   /* 02h */
    U8                      Reserved1[3];               /* 03h */
    U8                      MsgFlags;                   /* 04h */
    U32                     MsgContext;                 /* 08h */
} MSG_IOC_FACTS, MPI_POINTER PTR_IOC_FACTS,
  IOCFacts_t, MPI_POINTER pIOCFacts_t;

typedef struct _MPI_FW_VERSION_STRUCT
{
    U8                      Dev;                        /* 00h */
    U8                      Unit;                       /* 01h */
    U8                      Minor;                      /* 02h */
    U8                      Major;                      /* 03h */
} MPI_FW_VERSION_STRUCT;

typedef union _MPI_FW_VERSION
{
    MPI_FW_VERSION_STRUCT   Struct;
    U32                     Word;
} MPI_FW_VERSION;

/* IOC Facts Reply */
typedef struct _MSG_IOC_FACTS_REPLY
{
    U16                     MsgVersion;                 /* 00h */
    U8                      MsgLength;                  /* 02h */
    U8                      Function;                   /* 03h */
    U16                     HeaderVersion;              /* 04h */
    U8                      IOCNumber;                  /* 06h */
    U8                      MsgFlags;                   /* 07h */
    U32                     MsgContext;                 /* 08h */
    U16                     IOCExceptions;              /* 0Ch */
    U16                     IOCStatus;                  /* 0Eh */
    U32                     IOCLogInfo;                 /* 10h */
    U8                      MaxChainDepth;              /* 14h */
    U8                      WhoInit;                    /* 15h */
    U8                      BlockSize;                  /* 16h */
    U8                      Flags;                      /* 17h */
    U16                     ReplyQueueDepth;            /* 18h */
    U16                     RequestFrameSize;           /* 1Ah */
    U16                     Reserved_0101_FWVersion;    /* 1Ch */ /* obsolete 16-bit FWVersion */
    U16                     ProductID;                  /* 1Eh */
    U32                     CurrentHostMfaHighAddr;     /* 20h */
    U16                     GlobalCredits;              /* 24h */
    U8                      NumberOfPorts;              /* 26h */
    U8                      EventState;                 /* 27h */
    U32                     CurrentSenseBufferHighAddr; /* 28h */
    U16                     CurReplyFrameSize;          /* 2Ch */
    U8                      MaxDevices;                 /* 2Eh */
    U8                      MaxBuses;                   /* 2Fh */
    U32                     FWImageSize;                /* 30h */
    U32                     IOCCapabilities;            /* 34h */
    MPI_FW_VERSION          FWVersion;                  /* 38h */
    U16                     HighPriorityQueueDepth;     /* 3Ch */
    U16                     Reserved2;                  /* 3Eh */
    SGE_SIMPLE_UNION        HostPageBufferSGE;          /* 40h */
    U32                     ReplyFifoHostSignalingAddr; /* 4Ch */
} MSG_IOC_FACTS_REPLY, MPI_POINTER PTR_MSG_IOC_FACTS_REPLY,
  IOCFactsReply_t, MPI_POINTER pIOCFactsReply_t;

#define MPI_IOCFACTS_MSGVERSION_MAJOR_MASK              (0xFF00)
#define MPI_IOCFACTS_MSGVERSION_MAJOR_SHIFT             (8)
#define MPI_IOCFACTS_MSGVERSION_MINOR_MASK              (0x00FF)
#define MPI_IOCFACTS_MSGVERSION_MINOR_SHIFT             (0)

#define MPI_IOCFACTS_HDRVERSION_UNIT_MASK               (0xFF00)
#define MPI_IOCFACTS_HDRVERSION_UNIT_SHIFT              (8)
#define MPI_IOCFACTS_HDRVERSION_DEV_MASK                (0x00FF)
#define MPI_IOCFACTS_HDRVERSION_DEV_SHIFT               (0)

#define MPI_IOCFACTS_EXCEPT_CONFIG_CHECKSUM_FAIL        (0x0001)
#define MPI_IOCFACTS_EXCEPT_RAID_CONFIG_INVALID         (0x0002)
#define MPI_IOCFACTS_EXCEPT_FW_CHECKSUM_FAIL            (0x0004)
#define MPI_IOCFACTS_EXCEPT_PERSISTENT_TABLE_FULL       (0x0008)
#define MPI_IOCFACTS_EXCEPT_METADATA_UNSUPPORTED        (0x0010)

#define MPI_IOCFACTS_FLAGS_FW_DOWNLOAD_BOOT             (0x01)
#define MPI_IOCFACTS_FLAGS_REPLY_FIFO_HOST_SIGNAL       (0x02)
#define MPI_IOCFACTS_FLAGS_HOST_PAGE_BUFFER_PERSISTENT  (0x04)

#define MPI_IOCFACTS_EVENTSTATE_DISABLED                (0x00)
#define MPI_IOCFACTS_EVENTSTATE_ENABLED                 (0x01)

#define MPI_IOCFACTS_CAPABILITY_HIGH_PRI_Q              (0x00000001)
#define MPI_IOCFACTS_CAPABILITY_REPLY_HOST_SIGNAL       (0x00000002)
#define MPI_IOCFACTS_CAPABILITY_QUEUE_FULL_HANDLING     (0x00000004)
#define MPI_IOCFACTS_CAPABILITY_DIAG_TRACE_BUFFER       (0x00000008)
#define MPI_IOCFACTS_CAPABILITY_SNAPSHOT_BUFFER         (0x00000010)
#define MPI_IOCFACTS_CAPABILITY_EXTENDED_BUFFER         (0x00000020)
#define MPI_IOCFACTS_CAPABILITY_EEDP                    (0x00000040)
#define MPI_IOCFACTS_CAPABILITY_BIDIRECTIONAL           (0x00000080)
#define MPI_IOCFACTS_CAPABILITY_MULTICAST               (0x00000100)
#define MPI_IOCFACTS_CAPABILITY_SCSIIO32                (0x00000200)
#define MPI_IOCFACTS_CAPABILITY_NO_SCSIIO16             (0x00000400)
#define MPI_IOCFACTS_CAPABILITY_TLR                     (0x00000800)

/*****************************************************************************
*
*        M e s s a g e    F u n c t i o n s
*              0x80 -> 0x8F reserved for private message use per product
*
*
*****************************************************************************/

#define MPI_FUNCTION_SCSI_IO_REQUEST                (0x00)
#define MPI_FUNCTION_SCSI_TASK_MGMT                 (0x01)
#define MPI_FUNCTION_IOC_INIT                       (0x02)
#define MPI_FUNCTION_IOC_FACTS                      (0x03)
#define MPI_FUNCTION_CONFIG                         (0x04)
#define MPI_FUNCTION_PORT_FACTS                     (0x05)
#define MPI_FUNCTION_PORT_ENABLE                    (0x06)
#define MPI_FUNCTION_EVENT_NOTIFICATION             (0x07)
#define MPI_FUNCTION_EVENT_ACK                      (0x08)
#define MPI_FUNCTION_FW_DOWNLOAD                    (0x09)
#define MPI_FUNCTION_TARGET_CMD_BUFFER_POST         (0x0A)
#define MPI_FUNCTION_TARGET_ASSIST                  (0x0B)
#define MPI_FUNCTION_TARGET_STATUS_SEND             (0x0C)
#define MPI_FUNCTION_TARGET_MODE_ABORT              (0x0D)
#define MPI_FUNCTION_FC_LINK_SRVC_BUF_POST          (0x0E)
#define MPI_FUNCTION_FC_LINK_SRVC_RSP               (0x0F)
#define MPI_FUNCTION_FC_EX_LINK_SRVC_SEND           (0x10)
#define MPI_FUNCTION_FC_ABORT                       (0x11)
#define MPI_FUNCTION_FW_UPLOAD                      (0x12)
#define MPI_FUNCTION_FC_COMMON_TRANSPORT_SEND       (0x13)
#define MPI_FUNCTION_FC_PRIMITIVE_SEND              (0x14)

#define MPI_FUNCTION_RAID_ACTION                    (0x15)
#define MPI_FUNCTION_RAID_SCSI_IO_PASSTHROUGH       (0x16)

#define MPI_FUNCTION_TOOLBOX                        (0x17)

#define MPI_FUNCTION_SCSI_ENCLOSURE_PROCESSOR       (0x18)

#define MPI_FUNCTION_MAILBOX                        (0x19)

#define MPI_FUNCTION_SMP_PASSTHROUGH                (0x1A)
#define MPI_FUNCTION_SAS_IO_UNIT_CONTROL            (0x1B)
#define MPI_FUNCTION_SATA_PASSTHROUGH               (0x1C)

#define MPI_FUNCTION_DIAG_BUFFER_POST               (0x1D)
#define MPI_FUNCTION_DIAG_RELEASE                   (0x1E)

#define MPI_FUNCTION_SCSI_IO_32                     (0x1F)

#define MPI_FUNCTION_LAN_SEND                       (0x20)
#define MPI_FUNCTION_LAN_RECEIVE                    (0x21)
#define MPI_FUNCTION_LAN_RESET                      (0x22)

#define MPI_FUNCTION_TARGET_ASSIST_EXTENDED         (0x23)
#define MPI_FUNCTION_TARGET_CMD_BUF_BASE_POST       (0x24)
#define MPI_FUNCTION_TARGET_CMD_BUF_LIST_POST       (0x25)

#define MPI_FUNCTION_INBAND_BUFFER_POST             (0x28)
#define MPI_FUNCTION_INBAND_SEND                    (0x29)
#define MPI_FUNCTION_INBAND_RSP                     (0x2A)
#define MPI_FUNCTION_INBAND_ABORT                   (0x2B)

#define MPI_FUNCTION_IOC_MESSAGE_UNIT_RESET         (0x40)
#define MPI_FUNCTION_IO_UNIT_RESET                  (0x41)
#define MPI_FUNCTION_HANDSHAKE                      (0x42)
#define MPI_FUNCTION_REPLY_FRAME_REMOVAL            (0x43)
#define MPI_FUNCTION_HOST_PAGEBUF_ACCESS_CONTROL    (0x44)

#define MPI_MSG_IOCTL           0x806D7069  // mpi
#define DATA_FROM_APP           0x01
#define SCSI_IO                 0x4000
#define DUAL_SGLS               0x8000

#define MPI_FW_HEADER_WHAT_SIGNATURE        (0x29232840)

/* defines for using the ProductId field */
#define MPI_FW_HEADER_PID_TYPE_MASK             (0xF000)
#define MPI_FW_HEADER_PID_TYPE_SCSI             (0x0000)
#define MPI_FW_HEADER_PID_TYPE_FC               (0x1000)
#define MPI_FW_HEADER_PID_TYPE_SAS              (0x2000)

/****************************************************************************/
/*  SCSI IO messages and associated structures                              */
/****************************************************************************/

typedef struct _MSG_SCSI_IO_REQUEST
{
    U8                      TargetID;           /* 00h */
    U8                      Bus;                /* 01h */
    U8                      ChainOffset;        /* 02h */
    U8                      Function;           /* 03h */
    U8                      CDBLength;          /* 04h */
    U8                      SenseBufferLength;  /* 05h */
    U8                      Reserved;           /* 06h */
    U8                      MsgFlags;           /* 07h */
    U32                     MsgContext;         /* 08h */
    U8                      LUN[8];             /* 0Ch */
    U32                     Control;            /* 14h */
    U8                      CDB[16];            /* 18h */
    U32                     DataLength;         /* 28h */
    U32                     SenseBufferLowAddr; /* 2Ch */
    SGE_IO_UNION            SGL;                /* 30h */
} MSG_SCSI_IO_REQUEST, MPI_POINTER PTR_MSG_SCSI_IO_REQUEST,
  SCSIIORequest_t, MPI_POINTER pSCSIIORequest_t;


/* SCSI IO MsgFlags bits */

#define MPI_SCSIIO_MSGFLGS_SENSE_WIDTH              (0x01)
#define MPI_SCSIIO_MSGFLGS_SENSE_WIDTH_32           (0x00)
#define MPI_SCSIIO_MSGFLGS_SENSE_WIDTH_64           (0x01)

#define MPI_SCSIIO_MSGFLGS_SENSE_LOCATION           (0x02)
#define MPI_SCSIIO_MSGFLGS_SENSE_LOC_HOST           (0x00)
#define MPI_SCSIIO_MSGFLGS_SENSE_LOC_IOC            (0x02)

#define MPI_SCSIIO_MSGFLGS_CMD_DETERMINES_DATA_DIR  (0x04)

/* SCSI IO LUN fields */

#define MPI_SCSIIO_LUN_FIRST_LEVEL_ADDRESSING   (0x0000FFFF)
#define MPI_SCSIIO_LUN_SECOND_LEVEL_ADDRESSING  (0xFFFF0000)
#define MPI_SCSIIO_LUN_THIRD_LEVEL_ADDRESSING   (0x0000FFFF)
#define MPI_SCSIIO_LUN_FOURTH_LEVEL_ADDRESSING  (0xFFFF0000)
#define MPI_SCSIIO_LUN_LEVEL_1_WORD             (0xFF00)
#define MPI_SCSIIO_LUN_LEVEL_1_DWORD            (0x0000FF00)

/* SCSI IO Control bits */

#define MPI_SCSIIO_CONTROL_DATADIRECTION_MASK   (0x03000000)
#define MPI_SCSIIO_CONTROL_NODATATRANSFER       (0x00000000)
#define MPI_SCSIIO_CONTROL_WRITE                (0x01000000)
#define MPI_SCSIIO_CONTROL_READ                 (0x02000000)

#define MPI_SCSIIO_CONTROL_ADDCDBLEN_MASK       (0x3C000000)
#define MPI_SCSIIO_CONTROL_ADDCDBLEN_SHIFT      (26)

#define MPI_SCSIIO_CONTROL_TASKATTRIBUTE_MASK   (0x00000700)
#define MPI_SCSIIO_CONTROL_SIMPLEQ              (0x00000000)
#define MPI_SCSIIO_CONTROL_HEADOFQ              (0x00000100)
#define MPI_SCSIIO_CONTROL_ORDEREDQ             (0x00000200)
#define MPI_SCSIIO_CONTROL_ACAQ                 (0x00000400)
#define MPI_SCSIIO_CONTROL_UNTAGGED             (0x00000500)
#define MPI_SCSIIO_CONTROL_NO_DISCONNECT        (0x00000700)

#define MPI_SCSIIO_CONTROL_TASKMANAGE_MASK      (0x00FF0000)
#define MPI_SCSIIO_CONTROL_OBSOLETE             (0x00800000)
#define MPI_SCSIIO_CONTROL_CLEAR_ACA_RSV        (0x00400000)
#define MPI_SCSIIO_CONTROL_TARGET_RESET         (0x00200000)
#define MPI_SCSIIO_CONTROL_LUN_RESET_RSV        (0x00100000)
#define MPI_SCSIIO_CONTROL_RESERVED             (0x00080000)
#define MPI_SCSIIO_CONTROL_CLR_TASK_SET_RSV     (0x00040000)
#define MPI_SCSIIO_CONTROL_ABORT_TASK_SET       (0x00020000)
#define MPI_SCSIIO_CONTROL_RESERVED2            (0x00010000)


/* SCSI IO reply structure */
typedef struct _MSG_SCSI_IO_REPLY
{
    U8                      TargetID;           /* 00h */
    U8                      Bus;                /* 01h */
    U8                      MsgLength;          /* 02h */
    U8                      Function;           /* 03h */
    U8                      CDBLength;          /* 04h */
    U8                      SenseBufferLength;  /* 05h */
    U8                      Reserved;           /* 06h */
    U8                      MsgFlags;           /* 07h */
    U32                     MsgContext;         /* 08h */
    U8                      SCSIStatus;         /* 0Ch */
    U8                      SCSIState;          /* 0Dh */
    U16                     IOCStatus;          /* 0Eh */
    U32                     IOCLogInfo;         /* 10h */
    U32                     TransferCount;      /* 14h */
    U32                     SenseCount;         /* 18h */
    U32                     ResponseInfo;       /* 1Ch */
    U16                     TaskTag;            /* 20h */
    U16                     Reserved1;          /* 22h */
} MSG_SCSI_IO_REPLY, MPI_POINTER PTR_MSG_SCSI_IO_REPLY,
  SCSIIOReply_t, MPI_POINTER pSCSIIOReply_t;

typedef struct
{
	SCSIIOReply_t	 reply;
	U8				 sense[32];
} SCSI_REPLY;


/* SCSI IO Reply SCSIStatus values (SAM-2 status codes) */

#define MPI_SCSI_STATUS_SUCCESS                 (0x00)
#define MPI_SCSI_STATUS_CHECK_CONDITION         (0x02)
#define MPI_SCSI_STATUS_CONDITION_MET           (0x04)
#define MPI_SCSI_STATUS_BUSY                    (0x08)
#define MPI_SCSI_STATUS_INTERMEDIATE            (0x10)
#define MPI_SCSI_STATUS_INTERMEDIATE_CONDMET    (0x14)
#define MPI_SCSI_STATUS_RESERVATION_CONFLICT    (0x18)
#define MPI_SCSI_STATUS_COMMAND_TERMINATED      (0x22)
#define MPI_SCSI_STATUS_TASK_SET_FULL           (0x28)
#define MPI_SCSI_STATUS_ACA_ACTIVE              (0x30)

#define MPI_SCSI_STATUS_FCPEXT_DEVICE_LOGGED_OUT    (0x80)
#define MPI_SCSI_STATUS_FCPEXT_NO_LINK              (0x81)
#define MPI_SCSI_STATUS_FCPEXT_UNASSIGNED           (0x82)


/* SCSI IO Reply SCSIState values */

#define MPI_SCSI_STATE_AUTOSENSE_VALID          (0x01)
#define MPI_SCSI_STATE_AUTOSENSE_FAILED         (0x02)
#define MPI_SCSI_STATE_NO_SCSI_STATUS           (0x04)
#define MPI_SCSI_STATE_TERMINATED               (0x08)
#define MPI_SCSI_STATE_RESPONSE_INFO_VALID      (0x10)
#define MPI_SCSI_STATE_QUEUE_TAG_REJECTED       (0x20)

/* SCSI IO Reply ResponseInfo values */
/* (FCP-1 RSP_CODE values and SPI-3 Packetized Failure codes) */

#define MPI_SCSI_RSP_INFO_FUNCTION_COMPLETE     (0x00000000)
#define MPI_SCSI_RSP_INFO_FCP_BURST_LEN_ERROR   (0x01000000)
#define MPI_SCSI_RSP_INFO_CMND_FIELDS_INVALID   (0x02000000)
#define MPI_SCSI_RSP_INFO_FCP_DATA_RO_ERROR     (0x03000000)
#define MPI_SCSI_RSP_INFO_TASK_MGMT_UNSUPPORTED (0x04000000)
#define MPI_SCSI_RSP_INFO_TASK_MGMT_FAILED      (0x05000000)
#define MPI_SCSI_RSP_INFO_SPI_LQ_INVALID_TYPE   (0x06000000)

#define MPI_SCSI_TASKTAG_UNKNOWN                (0xFFFF)

/*****************************************************************************
*
*       C o n f i g    M e s s a g e    a n d    S t r u c t u r e s
*
*****************************************************************************/

typedef struct _CONFIG_PAGE_HEADER
{
    U8                      PageVersion;                /* 00h */
    U8                      PageLength;                 /* 01h */
    U8                      PageNumber;                 /* 02h */
    U8                      PageType;                   /* 03h */
} CONFIG_PAGE_HEADER;

typedef struct _CONFIG_EXTENDED_PAGE_HEADER
{
    U8                  PageVersion;                /* 00h */
    U8                  Reserved1;                  /* 01h */
    U8                  PageNumber;                 /* 02h */
    U8                  PageType;                   /* 03h */
    U16                 ExtPageLength;              /* 04h */
    U8                  ExtPageType;                /* 06h */
    U8                  Reserved2;                  /* 07h */
} CONFIG_EXTENDED_PAGE_HEADER;

typedef struct _MSG_CONFIG
{
    U8                      Action;                     /* 00h */
    U8                      Reserved;                   /* 01h */
    U8                      ChainOffset;                /* 02h */
    U8                      Function;                   /* 03h */
    U16                     ExtPageLength;              /* 04h */
    U8                      ExtPageType;                /* 06h */
    U8                      MsgFlags;                   /* 07h */
    U32                     MsgContext;                 /* 08h */
    U8                      Reserved2[8];               /* 0Ch */
    CONFIG_PAGE_HEADER      Header;                     /* 14h */
    U32                     PageAddress;                /* 18h */
    SGE_IO_UNION            PageBufferSGE;              /* 1Ch */
} MSG_CONFIG;

/* Config Reply Message */
typedef struct _MSG_CONFIG_REPLY
{
    U8                      Action;                     /* 00h */
    U8                      Reserved;                   /* 01h */
    U8                      MsgLength;                  /* 02h */
    U8                      Function;                   /* 03h */
    U16                     ExtPageLength;              /* 04h */
    U8                      ExtPageType;                /* 06h */
    U8                      MsgFlags;                   /* 07h */
    U32                     MsgContext;                 /* 08h */
    U8                      Reserved2[2];               /* 0Ch */
    U16                     IOCStatus;                  /* 0Eh */
    U32                     IOCLogInfo;                 /* 10h */
    CONFIG_PAGE_HEADER      Header;                     /* 14h */
} MSG_CONFIG_REPLY;

/****************************************************************************
*   Action field values
****************************************************************************/
#define MPI_CONFIG_ACTION_PAGE_HEADER               (0x00)
#define MPI_CONFIG_ACTION_PAGE_READ_CURRENT         (0x01)
#define MPI_CONFIG_ACTION_PAGE_WRITE_CURRENT        (0x02)
#define MPI_CONFIG_ACTION_PAGE_DEFAULT              (0x03)
#define MPI_CONFIG_ACTION_PAGE_WRITE_NVRAM          (0x04)
#define MPI_CONFIG_ACTION_PAGE_READ_DEFAULT         (0x05)
#define MPI_CONFIG_ACTION_PAGE_READ_NVRAM           (0x06)

/****************************************************************************
*   PageType field values
****************************************************************************/
#define MPI_CONFIG_PAGEATTR_READ_ONLY               (0x00)
#define MPI_CONFIG_PAGEATTR_CHANGEABLE              (0x10)
#define MPI_CONFIG_PAGEATTR_PERSISTENT              (0x20)
#define MPI_CONFIG_PAGEATTR_RO_PERSISTENT           (0x30)
#define MPI_CONFIG_PAGEATTR_MASK                    (0xF0)

#define MPI_CONFIG_PAGETYPE_IO_UNIT                 (0x00)
#define MPI_CONFIG_PAGETYPE_IOC                     (0x01)
#define MPI_CONFIG_PAGETYPE_BIOS                    (0x02)
#define MPI_CONFIG_PAGETYPE_SCSI_PORT               (0x03)
#define MPI_CONFIG_PAGETYPE_SCSI_DEVICE             (0x04)
#define MPI_CONFIG_PAGETYPE_FC_PORT                 (0x05)
#define MPI_CONFIG_PAGETYPE_FC_DEVICE               (0x06)
#define MPI_CONFIG_PAGETYPE_LAN                     (0x07)
#define MPI_CONFIG_PAGETYPE_RAID_VOLUME             (0x08)
#define MPI_CONFIG_PAGETYPE_MANUFACTURING           (0x09)
#define MPI_CONFIG_PAGETYPE_RAID_PHYSDISK           (0x0A)
#define MPI_CONFIG_PAGETYPE_INBAND                  (0x0B)
#define MPI_CONFIG_PAGETYPE_EXTENDED                (0x0F)
#define MPI_CONFIG_PAGETYPE_MASK                    (0x0F)

#define MPI_CONFIG_TYPENUM_MASK                     (0x0FFF)


/****************************************************************************
*   ExtPageType field values
****************************************************************************/
#define MPI_CONFIG_EXTPAGETYPE_SAS_IO_UNIT          (0x10)
#define MPI_CONFIG_EXTPAGETYPE_SAS_EXPANDER         (0x11)
#define MPI_CONFIG_EXTPAGETYPE_SAS_DEVICE           (0x12)
#define MPI_CONFIG_EXTPAGETYPE_SAS_PHY              (0x13)
#define MPI_CONFIG_EXTPAGETYPE_LOG                  (0x14)
#define MPI_CONFIG_EXTPAGETYPE_ENCLOSURE            (0x15)

/****************************************************************************
*   SAS Device Config Pages
****************************************************************************/

typedef struct _CONFIG_PAGE_SAS_DEVICE_0
{
    CONFIG_EXTENDED_PAGE_HEADER         Header;                 /* 00h */
    U16                                 Slot;                   /* 08h */
    U16                                 EnclosureHandle;        /* 0Ah */
    U64                                 SASAddress;             /* 0Ch */
    U16                                 ParentDevHandle;        /* 14h */
    U8                                  PhyNum;                 /* 16h */
    U8                                  AccessStatus;           /* 17h */
    U16                                 DevHandle;              /* 18h */
    U8                                  TargetID;               /* 1Ah */
    U8                                  Bus;                    /* 1Bh */
    U32                                 DeviceInfo;             /* 1Ch */
    U16                                 Flags;                  /* 20h */
    U8                                  PhysicalPort;           /* 22h */
    U8                                  Reserved2;              /* 23h */
} CONFIG_PAGE_SAS_DEVICE_0;

/****************************************************************************
*   PageAddress field values
****************************************************************************/
#define MPI_SCSI_PORT_PGAD_PORT_MASK                (0x000000FF)

#define MPI_SCSI_DEVICE_FORM_MASK                   (0xF0000000)
#define MPI_SCSI_DEVICE_FORM_BUS_TID                (0x00000000)
#define MPI_SCSI_DEVICE_TARGET_ID_MASK              (0x000000FF)
#define MPI_SCSI_DEVICE_TARGET_ID_SHIFT             (0)
#define MPI_SCSI_DEVICE_BUS_MASK                    (0x0000FF00)
#define MPI_SCSI_DEVICE_BUS_SHIFT                   (8)
#define MPI_SCSI_DEVICE_FORM_TARGET_MODE            (0x10000000)
#define MPI_SCSI_DEVICE_TM_RESPOND_ID_MASK          (0x000000FF)
#define MPI_SCSI_DEVICE_TM_RESPOND_ID_SHIFT         (0)
#define MPI_SCSI_DEVICE_TM_BUS_MASK                 (0x0000FF00)
#define MPI_SCSI_DEVICE_TM_BUS_SHIFT                (8)
#define MPI_SCSI_DEVICE_TM_INIT_ID_MASK             (0x00FF0000)
#define MPI_SCSI_DEVICE_TM_INIT_ID_SHIFT            (16)

#define MPI_FC_PORT_PGAD_PORT_MASK                  (0xF0000000)
#define MPI_FC_PORT_PGAD_PORT_SHIFT                 (28)
#define MPI_FC_PORT_PGAD_FORM_MASK                  (0x0F000000)
#define MPI_FC_PORT_PGAD_FORM_INDEX                 (0x01000000)
#define MPI_FC_PORT_PGAD_INDEX_MASK                 (0x0000FFFF)
#define MPI_FC_PORT_PGAD_INDEX_SHIFT                (0)

#define MPI_FC_DEVICE_PGAD_PORT_MASK                (0xF0000000)
#define MPI_FC_DEVICE_PGAD_PORT_SHIFT               (28)
#define MPI_FC_DEVICE_PGAD_FORM_MASK                (0x0F000000)
#define MPI_FC_DEVICE_PGAD_FORM_NEXT_DID            (0x00000000)
#define MPI_FC_DEVICE_PGAD_ND_PORT_MASK             (0xF0000000)
#define MPI_FC_DEVICE_PGAD_ND_PORT_SHIFT            (28)
#define MPI_FC_DEVICE_PGAD_ND_DID_MASK              (0x00FFFFFF)
#define MPI_FC_DEVICE_PGAD_ND_DID_SHIFT             (0)
#define MPI_FC_DEVICE_PGAD_FORM_BUS_TID             (0x01000000)
#define MPI_FC_DEVICE_PGAD_BT_BUS_MASK              (0x0000FF00)
#define MPI_FC_DEVICE_PGAD_BT_BUS_SHIFT             (8)
#define MPI_FC_DEVICE_PGAD_BT_TID_MASK              (0x000000FF)
#define MPI_FC_DEVICE_PGAD_BT_TID_SHIFT             (0)

#define MPI_PHYSDISK_PGAD_PHYSDISKNUM_MASK          (0x000000FF)
#define MPI_PHYSDISK_PGAD_PHYSDISKNUM_SHIFT         (0)

#define MPI_SAS_EXPAND_PGAD_FORM_MASK             (0xF0000000)
#define MPI_SAS_EXPAND_PGAD_FORM_SHIFT            (28)
#define MPI_SAS_EXPAND_PGAD_FORM_GET_NEXT_HANDLE  (0x00000000)
#define MPI_SAS_EXPAND_PGAD_FORM_HANDLE_PHY_NUM   (0x00000001)
#define MPI_SAS_EXPAND_PGAD_FORM_HANDLE           (0x00000002)
#define MPI_SAS_EXPAND_PGAD_GNH_MASK_HANDLE       (0x0000FFFF)
#define MPI_SAS_EXPAND_PGAD_GNH_SHIFT_HANDLE      (0)
#define MPI_SAS_EXPAND_PGAD_HPN_MASK_PHY          (0x00FF0000)
#define MPI_SAS_EXPAND_PGAD_HPN_SHIFT_PHY         (16)
#define MPI_SAS_EXPAND_PGAD_HPN_MASK_HANDLE       (0x0000FFFF)
#define MPI_SAS_EXPAND_PGAD_HPN_SHIFT_HANDLE      (0)
#define MPI_SAS_EXPAND_PGAD_H_MASK_HANDLE         (0x0000FFFF)
#define MPI_SAS_EXPAND_PGAD_H_SHIFT_HANDLE        (0)

#define MPI_SAS_DEVICE_PGAD_FORM_MASK               (0xF0000000)
#define MPI_SAS_DEVICE_PGAD_FORM_SHIFT              (28)
#define MPI_SAS_DEVICE_PGAD_FORM_GET_NEXT_HANDLE    (0x00000000)
#define MPI_SAS_DEVICE_PGAD_FORM_BUS_TARGET_ID      (0x00000001)
#define MPI_SAS_DEVICE_PGAD_FORM_HANDLE             (0x00000002)
#define MPI_SAS_DEVICE_PGAD_GNH_HANDLE_MASK         (0x0000FFFF)
#define MPI_SAS_DEVICE_PGAD_GNH_HANDLE_SHIFT        (0)
#define MPI_SAS_DEVICE_PGAD_BT_BUS_MASK             (0x0000FF00)
#define MPI_SAS_DEVICE_PGAD_BT_BUS_SHIFT            (8)
#define MPI_SAS_DEVICE_PGAD_BT_TID_MASK             (0x000000FF)
#define MPI_SAS_DEVICE_PGAD_BT_TID_SHIFT            (0)
#define MPI_SAS_DEVICE_PGAD_H_HANDLE_MASK           (0x0000FFFF)
#define MPI_SAS_DEVICE_PGAD_H_HANDLE_SHIFT          (0)

#define MPI_SAS_PHY_PGAD_FORM_MASK                  (0xF0000000)
#define MPI_SAS_PHY_PGAD_FORM_SHIFT                 (28)
#define MPI_SAS_PHY_PGAD_FORM_PHY_NUMBER            (0x0)
#define MPI_SAS_PHY_PGAD_FORM_PHY_TBL_INDEX         (0x1)
#define MPI_SAS_PHY_PGAD_PHY_NUMBER_MASK            (0x000000FF)
#define MPI_SAS_PHY_PGAD_PHY_NUMBER_SHIFT           (0)
#define MPI_SAS_PHY_PGAD_PHY_TBL_INDEX_MASK         (0x0000FFFF)
#define MPI_SAS_PHY_PGAD_PHY_TBL_INDEX_SHIFT        (0)

#define MPI_SAS_ENCLOS_PGAD_FORM_MASK               (0xF0000000)
#define MPI_SAS_ENCLOS_PGAD_FORM_SHIFT              (28)
#define MPI_SAS_ENCLOS_PGAD_FORM_GET_NEXT_HANDLE    (0x00000000)
#define MPI_SAS_ENCLOS_PGAD_FORM_HANDLE             (0x00000001)
#define MPI_SAS_ENCLOS_PGAD_GNH_HANDLE_MASK         (0x0000FFFF)
#define MPI_SAS_ENCLOS_PGAD_GNH_HANDLE_SHIFT        (0)
#define MPI_SAS_ENCLOS_PGAD_H_HANDLE_MASK           (0x0000FFFF)
#define MPI_SAS_ENCLOS_PGAD_H_HANDLE_SHIFT          (0)

/*****************************************************************************
*
*               I O C    S t a t u s   V a l u e s
*
*****************************************************************************/

/****************************************************************************/
/*  Common IOCStatus values for all replies                                 */
/****************************************************************************/

#define MPI_IOCSTATUS_SUCCESS                   (0x0000)
#define MPI_IOCSTATUS_INVALID_FUNCTION          (0x0001)
#define MPI_IOCSTATUS_BUSY                      (0x0002)
#define MPI_IOCSTATUS_INVALID_SGL               (0x0003)
#define MPI_IOCSTATUS_INTERNAL_ERROR            (0x0004)
#define MPI_IOCSTATUS_RESERVED                  (0x0005)
#define MPI_IOCSTATUS_INSUFFICIENT_RESOURCES    (0x0006)
#define MPI_IOCSTATUS_INVALID_FIELD             (0x0007)
#define MPI_IOCSTATUS_INVALID_STATE             (0x0008)
#define MPI_IOCSTATUS_OP_STATE_NOT_SUPPORTED    (0x0009)

/****************************************************************************/
/*  Config IOCStatus values                                                 */
/****************************************************************************/

#define MPI_IOCSTATUS_CONFIG_INVALID_ACTION     (0x0020)
#define MPI_IOCSTATUS_CONFIG_INVALID_TYPE       (0x0021)
#define MPI_IOCSTATUS_CONFIG_INVALID_PAGE       (0x0022)
#define MPI_IOCSTATUS_CONFIG_INVALID_DATA       (0x0023)
#define MPI_IOCSTATUS_CONFIG_NO_DEFAULTS        (0x0024)
#define MPI_IOCSTATUS_CONFIG_CANT_COMMIT        (0x0025)

/****************************************************************************/
/*  SCSIIO Reply (SPI & FCP) initiator values                               */
/****************************************************************************/

#define MPI_IOCSTATUS_SCSI_RECOVERED_ERROR      (0x0040)
#define MPI_IOCSTATUS_SCSI_INVALID_BUS          (0x0041)
#define MPI_IOCSTATUS_SCSI_INVALID_TARGETID     (0x0042)
#define MPI_IOCSTATUS_SCSI_DEVICE_NOT_THERE     (0x0043)
#define MPI_IOCSTATUS_SCSI_DATA_OVERRUN         (0x0044)
#define MPI_IOCSTATUS_SCSI_DATA_UNDERRUN        (0x0045)
#define MPI_IOCSTATUS_SCSI_IO_DATA_ERROR        (0x0046)
#define MPI_IOCSTATUS_SCSI_PROTOCOL_ERROR       (0x0047)
#define MPI_IOCSTATUS_SCSI_TASK_TERMINATED      (0x0048)
#define MPI_IOCSTATUS_SCSI_RESIDUAL_MISMATCH    (0x0049)
#define MPI_IOCSTATUS_SCSI_TASK_MGMT_FAILED     (0x004A)
#define MPI_IOCSTATUS_SCSI_IOC_TERMINATED       (0x004B)
#define MPI_IOCSTATUS_SCSI_EXT_TERMINATED       (0x004C)

/****************************************************************************/
/*  For use by SCSI Initiator and SCSI Target end-to-end data protection    */
/****************************************************************************/

#define MPI_IOCSTATUS_EEDP_GUARD_ERROR          (0x004D)
#define MPI_IOCSTATUS_EEDP_REF_TAG_ERROR        (0x004E)
#define MPI_IOCSTATUS_EEDP_APP_TAG_ERROR        (0x004F)


/****************************************************************************/
/*  SCSI Target values                                                      */
/****************************************************************************/

#define MPI_IOCSTATUS_TARGET_PRIORITY_IO         (0x0060)
#define MPI_IOCSTATUS_TARGET_INVALID_PORT        (0x0061)
#define MPI_IOCSTATUS_TARGET_INVALID_IOCINDEX    (0x0062)   /* obsolete name */
#define MPI_IOCSTATUS_TARGET_INVALID_IO_INDEX    (0x0062)
#define MPI_IOCSTATUS_TARGET_ABORTED             (0x0063)
#define MPI_IOCSTATUS_TARGET_NO_CONN_RETRYABLE   (0x0064)
#define MPI_IOCSTATUS_TARGET_NO_CONNECTION       (0x0065)
#define MPI_IOCSTATUS_TARGET_XFER_COUNT_MISMATCH (0x006A)
#define MPI_IOCSTATUS_TARGET_STS_DATA_NOT_SENT   (0x006B)
#define MPI_IOCSTATUS_TARGET_DATA_OFFSET_ERROR   (0x006D)
#define MPI_IOCSTATUS_TARGET_TOO_MUCH_WRITE_DATA (0x006E)
#define MPI_IOCSTATUS_TARGET_IU_TOO_SHORT        (0x006F)
#define MPI_IOCSTATUS_TARGET_ACK_NAK_TIMEOUT     (0x0070)
#define MPI_IOCSTATUS_TARGET_NAK_RECEIVED        (0x0071)

/****************************************************************************/
/*  Additional FCP target values (obsolete)                                 */
/****************************************************************************/

#define MPI_IOCSTATUS_TARGET_FC_ABORTED         (0x0066)    /* obsolete */
#define MPI_IOCSTATUS_TARGET_FC_RX_ID_INVALID   (0x0067)    /* obsolete */
#define MPI_IOCSTATUS_TARGET_FC_DID_INVALID     (0x0068)    /* obsolete */
#define MPI_IOCSTATUS_TARGET_FC_NODE_LOGGED_OUT (0x0069)    /* obsolete */

/****************************************************************************/
/*  Fibre Channel Direct Access values                                      */
/****************************************************************************/

#define MPI_IOCSTATUS_FC_ABORTED                (0x0066)
#define MPI_IOCSTATUS_FC_RX_ID_INVALID          (0x0067)
#define MPI_IOCSTATUS_FC_DID_INVALID            (0x0068)
#define MPI_IOCSTATUS_FC_NODE_LOGGED_OUT        (0x0069)
#define MPI_IOCSTATUS_FC_EXCHANGE_CANCELED      (0x006C)

/****************************************************************************/
/*  LAN values                                                              */
/****************************************************************************/

#define MPI_IOCSTATUS_LAN_DEVICE_NOT_FOUND      (0x0080)
#define MPI_IOCSTATUS_LAN_DEVICE_FAILURE        (0x0081)
#define MPI_IOCSTATUS_LAN_TRANSMIT_ERROR        (0x0082)
#define MPI_IOCSTATUS_LAN_TRANSMIT_ABORTED      (0x0083)
#define MPI_IOCSTATUS_LAN_RECEIVE_ERROR         (0x0084)
#define MPI_IOCSTATUS_LAN_RECEIVE_ABORTED       (0x0085)
#define MPI_IOCSTATUS_LAN_PARTIAL_PACKET        (0x0086)
#define MPI_IOCSTATUS_LAN_CANCELED              (0x0087)

/****************************************************************************/
/*  Serial Attached SCSI values                                             */
/****************************************************************************/

#define MPI_IOCSTATUS_SAS_SMP_REQUEST_FAILED    (0x0090)
#define MPI_IOCSTATUS_SAS_SMP_DATA_OVERRUN      (0x0091)

/****************************************************************************/
/*  Inband values                                                           */
/****************************************************************************/

#define MPI_IOCSTATUS_INBAND_ABORTED            (0x0098)
#define MPI_IOCSTATUS_INBAND_NO_CONNECTION      (0x0099)

/****************************************************************************/
/*  Diagnostic Tools values                                                 */
/****************************************************************************/

#define MPI_IOCSTATUS_DIAGNOSTIC_RELEASED       (0x00A0)


/****************************************************************************/
/*  IOCStatus flag to indicate that log info is available                   */
/****************************************************************************/

#define MPI_IOCSTATUS_FLAG_LOG_INFO_AVAILABLE   (0x8000)
#define MPI_IOCSTATUS_MASK                      (0x7FFF)

/****************************************************************************/
/*  LogInfo Types                                                           */
/****************************************************************************/

#define MPI_IOCLOGINFO_TYPE_MASK                (0xF0000000)
#define MPI_IOCLOGINFO_TYPE_SHIFT               (28)
#define MPI_IOCLOGINFO_TYPE_NONE                (0x0)
#define MPI_IOCLOGINFO_TYPE_SCSI                (0x1)
#define MPI_IOCLOGINFO_TYPE_FC                  (0x2)
#define MPI_IOCLOGINFO_TYPE_SAS                 (0x3)
#define MPI_IOCLOGINFO_TYPE_ISCSI               (0x4)
#define MPI_IOCLOGINFO_LOG_DATA_MASK            (0x0FFFFFFF)
