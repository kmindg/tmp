// netidparamsDef.h: Definition of the NETIDParams common structures.
//
//
// Copyright (C) 2000-2010	EMC Corporation
//
//
//
//	11 Mar 09	R. Hicks	DIMS 220711 - Added bGatewayValidAddr and bGlobalPrefixValidAddr fields
//							to IPV6_DATA.
//	05 May 09	E. Carter	DIMS 225160 - change script timeout from 10 mins. to 5 mins.
//	21 Jul 09	R. Hicks	DIMS 233092: define MAX_DEFAULT_GATEWAYS
//	30 Dec 09	D. Hesi		DIMS 217605: NAL/TLD Network Data Cache sync support
//	04 Mar 10	D. Hesi		ARS 351830: MTU caching support for 10G iSCSI Ports
//	19 Mar 10	D. Hesi		Coverity issue fixed
//	05 Apr 10	E. Carter	ARS 355744 - Add new Current IP address members to ADAPTER_REF &
//							up MAX_DEFAULT_GATEWAYS value to 100.
//	18 Oct 10	E. Carter	ARS 382903 - Added support for Flow Control.
//	16 Mar 11	E. Carter	AR 408040 - Eruption support
//	10 Sep 13   K. Zybin    AR 585874 - Support of multiple IPv6 per mgmt port

#include <stdio.h>
#include "csx_ext.h"
#include "UserDefs.h"
#include "CaptivePointer.h"
#include "K10TraceMsg.h"
#include "generic_types.h"

#ifdef ALAMOSA_WINDOWS_ENV
#include "iphlpapi.h"
#else
#include <ipexport.h>
#include <vector>
#endif /* ALAMOSA_WINDOWS_ENV - STDPORT */


#ifndef ALAMOSA_WINDOWS_ENV
#define CSX_STRUCT_EXPORT
#else
#define CSX_STRUCT_EXPORT CSX_CLASS_EXPORT
#endif /* ALAMOSA_WINDOWS_ENV - ODDCASE - some sort of export mess worth sorting out */

#ifndef NETIDParamsDef_H
#define NETIDParamsDef_H

//#define NETID_DEFAULT_NIC	1
//#define NETID_DEFAULT_PORT	1
#define IP_INVALID_PORT		0xFF
#define IP_PORT_MGMT		0xF0
#define IP_PORT_ISCSI		0xF1
#define IP_PORT_REPLICATION	0xF2

#define NO_PORT_MATCH		(-1)
#define MAX_NETSH_WAIT			300000			// 5 minutes
#define MAX_NETSH_WAIT_100_NANO CSX_CONST_S64(3000000000)	// 5 minutes
#define NAVI_GATEWAY_METRIC		10
#define iSCSI_GATEWAY_METRIC	20
#define INVALID_GATE_METRIC		(0xFFFFFFFF)

#define MAX_DEFAULT_GATEWAYS	100

// Application HRESULT error codes
#define NET_ERROR_NIC_DISABLE_FAILURE			0x80000400
#define NET_ERROR_NIC_ENABLE_FAILURE			0x80000401

#define NAVI_PORT_NAME			"ManagementPort0"
#define ISCSI_PORT_NAME_COMMON	"iSCSIPort"

// The maximum time (in secs.) allowed for a perl script to return without logging
// ktrace message.
#define NET_PERL_SCRIPT_EXEC_TRACE_TIME_LIMIT 5

#define CONVERT_FROM_IN6(ip6) ((ip6.u.Byte[0]<<24) | (ip6.u.Byte[1]<<16) |(ip6.u.Byte[2]<<8) |ip6.u.Byte[3])

// 24 bit Organizational Unique Identifier assigned to EMC by the IEEE. This value is currently 00:60:16.
// The Local/Universal bit must be set to 1. This make the value 02:60:16
#define CLARIION_OUI	(0x026016)
#define CLARIION_OUI_SIZE   24

typedef ULONG IPADDR;

#ifndef NIC_FLOW_CONTROL_XMIT_MASK
// FLOW_CONTROL_OPTION values are a bitmap with Receive at bit 1 and Transmit at bit 0
// Note that this enum is also in NetworkAdmin.h, and the two must be kept in sync.
enum FLOW_CONTROL_OPTION
{
	FLOW_CONTROL_XMIT_OFF_REC_OFF	= 0,
	FLOW_CONTROL_XMIT_OFF_REC_ON	= 1,
	FLOW_CONTROL_XMIT_ON_REC_OFF	= 2,
	FLOW_CONTROL_XMIT_ON_REC_ON		= 3,
	FLOW_CONTROL_XMIT_AUTO_REC_AUTO	= 0x80000000
};

#define NIC_FLOW_CONTROL_XMIT_MASK	1;
#define NIC_FLOW_CONTROL_REC_MASK	2;
#endif

enum NETID_ADAPTER_TYPE
{
	NETID_ADAPTER_TYPE_NONE = 0,
	NETID_ADAPTER_TYPE_BROADCOM_1GB,
	NETID_ADAPTER_TYPE_BROADCOM_10GB,
	NETID_ADAPTER_TYPE_BROADCOM_1GB_SUPERCELL,
	NETID_ADAPTER_TYPE_BROADCOM_10GB_ERUPTION,
	NETID_ADAPTER_TYPE_BROADCOM_VIRTUAL_PORT, 
	NETID_ADAPTER_TYPE_QLOGIC_1GB,
	NETID_ADAPTER_TYPE_QLOGIC_VIRTUAL_PORT, 
	NETID_ADAPTER_TYPE_VMWARE_1GB,
        NETID_ADAPTER_TYPE_BROADCOM_BC_10GB_ERUPTION,
	NETID_ADAPTER_TYPE_QLOGIC_10GB,
	NETID_ADAPTER_TYPE_QLOGIC_10GB_VIRTUAL_PORT, 
	NETID_ADAPTER_TYPE_UNKNOWN,
	NETID_ADAPTER_TYPE_MAX = NETID_ADAPTER_TYPE_UNKNOWN
};

#define IPV6_FORMAT(STR, IN6) sprintf(STR, "%02x%02x:%02x%02x:%02x%02x:%02x%02x:%02x%02x:%02x%02x:%02x%02x:%02x%02x" \
	, (unsigned int) IN6.u.Byte[0], (unsigned int) IN6.u.Byte[1] \
	, (unsigned int) IN6.u.Byte[2], (unsigned int) IN6.u.Byte[3] \
	, (unsigned int) IN6.u.Byte[4], (unsigned int) IN6.u.Byte[5] \
	, (unsigned int) IN6.u.Byte[6], (unsigned int) IN6.u.Byte[7] \
	, (unsigned int) IN6.u.Byte[8], (unsigned int) IN6.u.Byte[9] \
	, (unsigned int) IN6.u.Byte[10], (unsigned int) IN6.u.Byte[11] \
	, (unsigned int) IN6.u.Byte[12], (unsigned int) IN6.u.Byte[13] \
	, (unsigned int) IN6.u.Byte[14], (unsigned int) IN6.u.Byte[15]) \
	/**/

#define IPV6_IS_SAME_INTERFACE_ID(IN61, IN62) \
	( true \
		&& (IN61.u.Byte[8] == IN62.u.Byte[8]) \
		&& (IN61.u.Byte[9] == IN62.u.Byte[9]) \
		&& (IN61.u.Byte[10] == IN62.u.Byte[10]) \
		&& (IN61.u.Byte[11] == IN62.u.Byte[11]) \
		&& (IN61.u.Byte[12] == IN62.u.Byte[12]) \
		&& (IN61.u.Byte[13] == IN62.u.Byte[13]) \
		&& (IN61.u.Byte[14] == IN62.u.Byte[14]) \
		&& (IN61.u.Byte[15] == IN62.u.Byte[15]) \
	) \
	/**/

#define IPV6_MODE(VAL) ((VAL) == None ? ("None") : ((VAL) == Automatic ? "Automatic" : "Manual"))

#define IPV6_V2IP_ADAPTER_ADDRESSES(VAR) ((IP_ADAPTER_ADDRESSES *) (VAR))

#define IPv6_COMPARE_PREFIX(BITSTOCOMPARE, BITSTOCOMPAREWITH, NOB) \
	( \
		( ((unsigned int )( (unsigned int ) (BITSTOCOMPARE >> (64 - NOB)))) == ( (unsigned int ) ( (BITSTOCOMPAREWITH) >> ((32 - (NOB))%16) ) ) ) \
	) \
	/**/

#define IPv6_RESERVED_PREFIX(BITSTOCOMPARE, BITSOFPREFIX, NOB, ERROR) \
	{ \
		if(IPv6_COMPARE_PREFIX(BITSTOCOMPARE, BITSOFPREFIX, NOB)) \
		{ \
			THROW_K10_EXCEPTION_EX(ERROR, K10_GLOBALMGMT_ERROR_IPv6_RESERVED_PREFIX, (unsigned int) (BITSTOCOMPARE >> (64 - (NOB)))); \
		} \
	} \
	/**/

typedef UINT_64E IPV6_GLOBAL_PREFIX;
typedef UINT_64E IPV6_SUBNET_ID;
typedef UINT_64E IPV6_UPPER_ID;
typedef UINT_64E IPV6_INTERFACE_ID;

typedef IPV6_GLOBAL_PREFIX* PIPV6_GLOBAL_PREFIX;
typedef IPV6_SUBNET_ID* PIPV6_SUBNET_ID;
typedef IPV6_UPPER_ID* PIPV6_UPPER_ID;
typedef IPV6_INTERFACE_ID* PIPV6_INTERFACE_ID;

#define STR_MAX_ADDR_SIZE		64
#define MASK					11111
#define STR_LEN_5				5
#define STR_LEN_10				10
#define STR_LEN_128				128
#define STR_LEN_512				512
#define MAX_PREFIX_LEN			64
#define MAX_PREFIX_BUFFER_LEN	23

typedef struct _IPV4_DATA
{
	BOOL	bUseDHCP;
	ULONG	iIpAddr; 
	ULONG	iSubnetMask;
	ULONG	iGateway;
	char	chMachineName[STR_MAX_ADDR_SIZE];
	char	chDomainName[STR_MAX_ADDR_SIZE];
	//
	BOOL	bUseDHCPChanged;
	BOOL	bIpAddrChanged;
	BOOL	bSubnetMaskChanged;
	BOOL	bGatewayChanged;
	BOOL	bMachineNameChanged;
	BOOL	bDomainNameChanged;
	BOOL	bCommitted;
	BOOL    bDeleted;
}IPV4_DATA, *PIPV4_DATA;

/*
 * To support report of multiple ip assigned to mgmt port in auto mode (by DHCP and SLAAC)
 * The addtional data space is required. This data will be used only for get command, so not all
 * the parameters are need to be changed to array. The set command still operates with one ip per port
 * */
#ifdef ALAMOSA_WINDOWS_ENV
typedef struct _IPV6_DATA
{
	char			chIPv6Gateway[STR_MAX_ADDR_SIZE];
	char			chLinkLocalAddress[STR_MAX_ADDR_SIZE];
	char			chGlobalUnicastAddress[STR_MAX_ADDR_SIZE];
	char			chGlobalPrefix[STR_MAX_ADDR_SIZE];
	UINT_64E		ui64GlobalPrefix;
	unsigned int	uiGlobalPrefixLength;
	char			chSubnetId[STR_MAX_ADDR_SIZE];
	char			chInterfaceId[STR_MAX_ADDR_SIZE];
	unsigned int	uiIPv6Mode;

	IN6_ADDR		IPv6Gateway;
	IN6_ADDR		LinkLocalAddress;
	IN6_ADDR		GlobalUnicastAddress;

	ULONG			LinkLocalValidLifetime;
	ULONG			LinkLocalPreferredLifetime;
	ULONG			GlobalUnicastValidLifetime;
	ULONG			GlobalUnicastPreferredLifetime;
	//
	BOOL			bGlobalUnicastAddressChanged;
	BOOL			bGlobalUnicastValidAddr;
	BOOL			bIPv6GatewayChanged;
	BOOL			bGatewayValidAddr;
	BOOL			bGlobalPrefixChanged;
	BOOL			bGlobalPrefixValidAddr;
	BOOL			bGlobalPrefixLengthChanged;
	BOOL			bIPv6ModeChanged;

	BOOL			bLinkLocalValidLifetimeChanged;
	BOOL			bLinkLocalPreferredLifetimeChanged;
	BOOL			bGlobalUnicastValidLifetimeChanged;
	BOOL			bGlobalUnicastPreferredLifetimeChanged;

	BOOL			bCommitted;
	BOOL                    bDeleted;
}IPV6_DATA, *PIPV6_DATA;

//Stub typedef. just need to have common interface for windows and linux
typedef int IPV6_DATA_IPADDR_VECT;
#else
typedef struct _IPV6_DATA_IPADDR
{
	char			chIPv6Gateway[STR_MAX_ADDR_SIZE];
	char			chGlobalUnicastAddress[STR_MAX_ADDR_SIZE];
	char			chGlobalPrefix[STR_MAX_ADDR_SIZE];
	char			chSubnetId[STR_MAX_ADDR_SIZE];
	UINT_64E		ui64GlobalPrefix;
	UINT            uiGlobalPrefixLength;

	IN6_ADDR		IPv6Gateway;
	IN6_ADDR		GlobalUnicastAddress;
	//
	BOOL			bGlobalUnicastAddressChanged;
	BOOL			bGlobalUnicastValidAddr;
	BOOL			bIPv6GatewayChanged;
	BOOL			bGatewayValidAddr;
	BOOL			bGlobalPrefixChanged;
	BOOL			bGlobalPrefixValidAddr;
	BOOL			bGlobalPrefixLengthChanged;

	IN6_ADDR		LinkLocalAddress;
	char			chLinkLocalAddress[STR_MAX_ADDR_SIZE];
    //Consturctor
    _IPV6_DATA_IPADDR():
        ui64GlobalPrefix(0), 
        uiGlobalPrefixLength(0), 
        bGlobalUnicastAddressChanged(FALSE), 
        bGlobalUnicastValidAddr(FALSE), 
        bIPv6GatewayChanged(FALSE), 
        bGatewayValidAddr(FALSE), 
        bGlobalPrefixChanged(FALSE), 
        bGlobalPrefixValidAddr(FALSE), 
        bGlobalPrefixLengthChanged(FALSE)
    {
        memset(&IPv6Gateway, 0, sizeof(IPv6Gateway));
        memset(&GlobalUnicastAddress, 0, sizeof(GlobalUnicastAddress));
        memset(&LinkLocalAddress, 0, sizeof(LinkLocalAddress));
	    chIPv6Gateway[0] = 0;
	    chGlobalUnicastAddress[0] = 0;
	    chGlobalPrefix[0] = 0;
	    chSubnetId[0] = 0;
        chLinkLocalAddress[0] = 0;
    }
}IPV6_DATA_IPADDR, *PIPV6_DATA_IPADDR;

typedef std::vector<IPV6_DATA_IPADDR> IPV6_DATA_IPADDR_VECT;

typedef struct _IPV6_DATA
{
    IPV6_DATA_IPADDR_VECT ip;

	char			chInterfaceId[STR_MAX_ADDR_SIZE];
	UINT            uiIPv6Mode; //enum IPv6_Mode 


	ULONG			LinkLocalValidLifetime;
	ULONG			LinkLocalPreferredLifetime;
	ULONG			GlobalUnicastValidLifetime;
	ULONG			GlobalUnicastPreferredLifetime;
	//
	BOOL			bIPv6ModeChanged;
    //lifetime are not used outside ALAMOSA_WINDOWS_ENV
	BOOL			bLinkLocalValidLifetimeChanged;
	BOOL			bLinkLocalPreferredLifetimeChanged;
	BOOL			bGlobalUnicastValidLifetimeChanged;
	BOOL			bGlobalUnicastPreferredLifetimeChanged;

	BOOL			bCommitted;
	BOOL            bDeleted;
    //Constructor
    _IPV6_DATA():
        ip(1), 
        uiIPv6Mode(-1/*undefined*/), 
        LinkLocalValidLifetime(0), 
        LinkLocalPreferredLifetime(0), 
        GlobalUnicastValidLifetime(0), 
        GlobalUnicastPreferredLifetime(0), 
        bIPv6ModeChanged(FALSE), 
        bLinkLocalValidLifetimeChanged(FALSE), 
        bLinkLocalPreferredLifetimeChanged(FALSE), 
        bGlobalUnicastValidLifetimeChanged(FALSE), 
        bGlobalUnicastPreferredLifetimeChanged(FALSE), 
        bCommitted(FALSE), 
        bDeleted(FALSE)
    {
        chInterfaceId[0] = 0;
    } 
}IPV6_DATA, *PIPV6_DATA;
#endif

#ifdef ALAMOSA_WINDOWS_ENV
#define GET_MGMT_IPV6_DATA(portData) (portData).IPv6Data
#define GET_MGMT_IPV6_DATA_IT(portData, it) (portData).IPv6Data
#else
#define GET_MGMT_IPV6_DATA(portData) (portData).IPv6Data.ip[0]
#define GET_MGMT_IPV6_DATA_IT(portData, it) (*it)
#endif


typedef struct CSX_STRUCT_EXPORT _MGMT_PORT_DATA
{
	IPV4_DATA	IPv4Data;
	IPV6_DATA	IPv6Data;
	BOOL		bIsDataInitialized;
	char		chAdapterName[STR_LEN_128];

	BOOL		bVlanEnabled;
	ULONG		iVlanId;

	unsigned char	uchMacAddress[6];
	//
	BOOL		bVlanEnabledChanged;
	ULONG		bVlanIdChanged;

	BOOL	bCommitted;

	_MGMT_PORT_DATA()
	{
		bIsDataInitialized	= FALSE;
#ifdef ALAMOSA_WINDOWS_ENV
		IPv6Data.ui64GlobalPrefix	= 0;
#endif
		IPv6Data.uiIPv6Mode	= 0;
		strcpy(chAdapterName, "");
		bVlanEnabled = FALSE;
		iVlanId = 0;
		bVlanEnabledChanged = FALSE;
		bVlanIdChanged = FALSE;

		bCommitted = FALSE;
		IPv4Data.bCommitted = FALSE;
		IPv6Data.bCommitted = FALSE;
        chAdapterName[0] = 0;
        memset(&uchMacAddress, 0, sizeof(uchMacAddress));
        memset(&IPv4Data, 0, sizeof(IPv4Data));
	}
}MGMT_PORT_DATA, *PMGMT_PORT_DATA;

typedef struct CSX_STRUCT_EXPORT _ISCSI_PORT_DATA
{
	int			iLogicalPortID;
	ULONG		ulCurrentMTU;
	BOOL		bIsMTUValid;
	BOOL		bIsInitialized;

	_ISCSI_PORT_DATA()
	{
		iLogicalPortID	= -1;
		ulCurrentMTU	= 0;
		bIsMTUValid		= FALSE;
		bIsInitialized	= FALSE;
	}
}ISCSI_PORT_DATA, *PISCSI_PORT_DATA;

enum IPv6_Mode { Undefined=-1, None=0, Automatic, Manual };

// Use to to hold data until caller issues a commit function call
typedef struct _PENDING_SET_DATA
{
	BOOL				HasVlanChanged;
	BOOL				bVlanEnabled;
	DWORD				VlanId;

	BOOL				HasIPv4Changed;
	CPtr <char>			cpDNS;
	CPtr <char>			cpDefaultGateway;
	CPtr <char>			cpIP;
	CPtr <char>			cpSubnet;
	DWORD				GatewayMetric;
	BOOL				bUseDHCP;

	// IPv6
	BOOL				HasIPv6Changed;
	IPv6_Mode			IPv6_Allot_Mode;

	// Manual IPv6 change
	CPtr <char>			cpIPv6Gateway;
	CPtr <char>			cpIPv6SubnetId;
	CPtr <char>			cpIPv6GlobalPrefix;
	unsigned int		uiIPv6PrefixLength;

} PENDING_SET_DATA, *PPENDING_SET_DATA;

// We need the Nice Name here because it is not in IP_ADAPTER_INFO.
// In that struct:	AdapterName	- is an OS UID string (like a DLL ID)
//					Description - is the name of the HBA (e.g. QLOGIC XXX)

typedef struct _AdapterRef {
	unsigned char			Port;
	unsigned char			LogicalPort;
	unsigned char			VirtualPort;

	unsigned char			PCI_Bus;
	unsigned char			PCI_Device;
	unsigned char			PCI_Function;

	CPtr <char>				cpPnpInstance;
	CPtr <char>				cpNiceName;
	IP_ADAPTER_INFO			*pAdapterInfo;
	void					*pAdapterAddresses;  // IP_ADAPTER_ADDRESSES
	IPADDR					IPv4ConfiguredIPAddress;
	UINT					IPv4ConfiguredPrefixLength;
	IPv6_Mode				IPv6_Allot_Mode;
	unsigned int			uiIPv6PrefixLength;
	CPtr <char>				cpIPv6Gateway;
	PENDING_SET_DATA		PendingSetData;
}ADAPTER_REF, *PADAPTER_REF;

typedef
struct _TagMapIpAddr{

	csx_uchar_t byte[4];

}MapIpAddr;


// Memory mapping to synchronization Network Data between TLD and NAL
// Memory mapping file name
#define NETWORK_SYNC_MAPPING_FILE_NAME "K10NetworkDataSync"

enum NETWORK_CACHE_SYNC_MODULE_NAME
{
	MODULE_NONE = 0,
	MODULE_TLD,
	MODULE_NAL
};

typedef struct _NetworkCacheSyncData
{
	unsigned int uiModuleName;
	bool bInSync;
} NetworkCacheSyncData;

typedef struct CSX_STRUCT_EXPORT _ISCSICacheSyncData
{
	unsigned int uiInitilizedPortCount;
	_ISCSICacheSyncData()
	{
		uiInitilizedPortCount = 0;
	}
} ISCSICacheSyncData;

#endif
