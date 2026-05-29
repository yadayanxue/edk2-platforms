/** @file
  Definitions for MicroChip Smsc95xx Ethernet adapter.

  Copyright (c) 2011 - 2015, Intel Corporation. All rights reserved.
  Copyright (c) 2020, ARM Limited. All rights reserved.

  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#pragma once

#include <Uefi.h>

#include <Guid/EventGroup.h>

#include <IndustryStandard/Pci.h>

#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>
#include <Library/DevicePathLib.h>
#include <Library/NetLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiDriverEntryPoint.h>
#include <Library/UefiLib.h>
#include <Library/FdtLib.h>
#include <Library/UefiUsbLib.h>

#include <Protocol/DevicePath.h>
#include <Protocol/LoadedImage.h>
#include <Protocol/NetworkInterfaceIdentifier.h>
#include <Protocol/SimpleNetwork.h>
#include <Protocol/UsbIo.h>


#define REPORTLINK    1


//------------------------------------------------------------------------------
//  Macros
//------------------------------------------------------------------------------

/**
  TURBO_MODE 多包突发模式，注意burst cap值过小会丢包，要保证缓存大小能容纳以太网帧
  以太网头部 14字节 + IP包数据载荷(MTU) 1500字节 + 帧校验(FCS) 4字节 = 1518字节
  IP包 : IP头部20字节 + UDP头部8字节 + TFTP协议头部4字节 + TFTP数据载荷 1468字节 = 1500字节

  tftp -s 16384, bs 匹配 lan9514 缓存设置 16384，16k
  FS HS不同速度下的Burst Cap值需要调整，匹配缓存大小
  单包模式下，tftp -s bs ，block size 超过1468字节时，会分包多次bulkin接收。
**/
#define TURBO_MODE

#define ETH_P_8021Q 0x8100 /* 802.1Q VLAN Extended Header  */

#define HS_USB_PKT_SIZE (512)
#define FS_USB_PKT_SIZE (64)
#define DEFAULT_HS_BURST_CAP_SIZE (8 * 1024 + 5 * HS_USB_PKT_SIZE)
#define DEFAULT_FS_BURST_CAP_SIZE (3 * 1024 + 33 * FS_USB_PKT_SIZE)
// 最小工作配置
// #define DEFAULT_HS_BURST_CAP_SIZE (5 * HS_USB_PKT_SIZE)
// #define DEFAULT_FS_BURST_CAP_SIZE (33 * FS_USB_PKT_SIZE)
#define DEFAULT_BULK_IN_DELAY (0x00002000)
#define MAX_SINGLE_PACKET_SIZE (2048)
#define LAN95XX_EEPROM_MAGIC (0x9500)
#define EEPROM_MAC_OFFSET (0x01)
#define DEFAULT_TX_CSUM_ENABLE (true)
#define DEFAULT_RX_CSUM_ENABLE (true)
#define SMSC95XX_TX_OVERHEAD (8)
#define SMSC95XX_TX_OVERHEAD_CSUM (12)
#define SUPPORTED_WAKE (WAKE_PHY | WAKE_UCAST | WAKE_BCAST | \
                        WAKE_MCAST | WAKE_ARP | WAKE_MAGIC)

#define FEATURE_8_WAKEUP_FILTERS (0x01)
#define FEATURE_PHY_NLP_CROSSOVER (0x02)
#define FEATURE_REMOTE_WAKEUP (0x04)

#define SUSPEND_SUSPEND0 (0x01)
#define SUSPEND_SUSPEND1 (0x02)
#define SUSPEND_SUSPEND2 (0x04)
#define SUSPEND_SUSPEND3 (0x08)
#define SUSPEND_ALLMODES (SUSPEND_SUSPEND0 | SUSPEND_SUSPEND1 | \
                          SUSPEND_SUSPEND2 | SUSPEND_SUSPEND3)

#define SMSC95XX_NR_IRQS (1) /* raise to 12 for GPIOs */
#define PHY_HWIRQ (SMSC95XX_NR_IRQS - 1)

#define USB_IS_IN_ENDPOINT(EndPointAddr)      (((EndPointAddr) & BIT7) != 0)  ///<  Return TRUE/FALSE for IN direction
#define USB_IS_OUT_ENDPOINT(EndPointAddr)     (((EndPointAddr) & BIT7) == 0)  ///<  Return TRUE/FALSE for OUT direction
#define USB_IS_BULK_ENDPOINT(Attribute)       (((Attribute) & (BIT0 | BIT1)) == USB_ENDPOINT_BULK)      ///<  Return TRUE/FALSE for BULK type
#define USB_IS_INTERRUPT_ENDPOINT(Attribute)  (((Attribute) & (BIT0 | BIT1)) == USB_ENDPOINT_INTERRUPT) ///<  Return TRUE/FALSE for INTERRUPT type

//------------------------------------------------------------------------------
//  Constants
//------------------------------------------------------------------------------

#define DEBUG_RX_BROADCAST  0x40000000  ///<  Display RX broadcast messages
#define DEBUG_RX_MULTICAST  0x20000000  ///<  Display RX multicast messages
#define DEBUG_RX_UNICAST    0x10000000  ///<  Display RX unicast messages
#define DEBUG_MAC_ADDRESS   0x08000000  ///<  Display the MAC address
#define DEBUG_LINK          0x04000000  ///<  Display the link status
#define DEBUG_TX            0x02000000  ///<  Display the TX messages
#define DEBUG_PHY           0x01000000  ///<  Display the PHY register values
#define DEBUG_SROM          0x00800000  ///<  Display the SROM contents
#define DEBUG_TIMER         0x00400000  ///<  Display the timer routine entry/exit
#define DEBUG_TPL           0x00200000  ///<  Display the timer routine entry/exit


#define ETHERNET_HEADER_SIZE  sizeof (ETHERNET_HEADER)  ///<  Size in bytes of the Ethernet header
#define MIN_ETHERNET_PKT_SIZE 64    // 包括以太网头部14字节和帧校验4字节，最小数据载荷46字节
#define MAX_ETHERNET_PKT_SIZE 1500  // MTU，最大数据载荷1500字节，不包括以太网头部和帧校验

#define USB_NETWORK_CLASS   0x09    ///<  USB Network class code
#define USB_BUS_TIMEOUT     1000    ///<  USB timeout in milliseconds

#define USB_MAX_BULKIN_SIZE   sizeof(RX_PACKET)
#define USB_MAX_BULKOUT_SIZE  sizeof(TX_PACKET)

#define USB_MAX_PKT_TX_SIZE   (512*3)

#ifdef TURBO_MODE
#define USB_MAX_PKT_RX_SIZE   DEFAULT_HS_BURST_CAP_SIZE
#else
#define USB_MAX_PKT_RX_SIZE   (512*3)
#endif

#define HC_DEBUG        0
#define BULKIN_TIMEOUT  3000
#define AUTONEG_DELAY   2000000

/**
  Verify new TPL value

  This macro which is enabled when debug is enabled verifies that
  the new TPL value is >= the current TPL value.
**/
#ifdef VERIFY_TPL
#undef VERIFY_TPL
#endif  //  VERIFY_TPL

#if !defined(MDEPKG_NDEBUG)

#define VERIFY_TPL(tpl)                         \
{                                               \
  EFI_TPL PreviousTpl;                          \
                                                \
  PreviousTpl = gBS->RaiseTPL (TPL_HIGH_LEVEL); \
  gBS->RestoreTPL (PreviousTpl);                \
  if (PreviousTpl > tpl) {                      \
    DEBUG ((DEBUG_ERROR, "Current TPL: %d, New TPL: %d\r\n", PreviousTpl, tpl));  \
    ASSERT (PreviousTpl <= tpl);                \
  }                                             \
}

#else   //  MDEPKG_NDEBUG

#define VERIFY_TPL(tpl)

#endif  //  MDEPKG_NDEBUG

//------------------------------------------------------------------------------
//  Hardware Definition
//------------------------------------------------------------------------------

#define DEV_SIGNATURE     SIGNATURE_32 ('S','M','S','C')  ///<  Signature of data structures in memory

#define VENDOR_ID         0x0424  ///<  Vendor ID for SMSC
#define PRODUCT_ID        0xec00 // LAN9512/LAN9514 Ethernet

#define RESET_MSEC        1000    ///<  Reset duration
#define PHY_RESET_MSEC    500     ///<  PHY reset duration

//
//  RX Control register
//

#define RXC_PRO           0x0001  ///<  Receive all packets
#define RXC_AMALL         0x0002  ///<  Receive all multicast packets
#define RXC_SEP           0x0004  ///<  Save error packets
#define RXC_AB            0x0008  ///<  Receive broadcast packets
#define RXC_AM            0x0010  ///<  Use multicast destination address hash table
#define RXC_AP            0x0020  ///<  Accept physical address from Multicast Filter
#define RXC_SO            0x0080  ///<  Start operation
#define RXC_MFB           0x0300  ///<  Maximum frame burst
#define RXC_MFB_2048      0       ///<  Maximum frame size:  2048 bytes
#define RXC_MFB_4096      0x0100  ///<  Maximum frame size:  4096 bytes
#define RXC_MFB_8192      0x0200  ///<  Maximum frame size:  8192 bytes
#define RXC_MFB_16384     0x0300  ///<  Maximum frame size: 16384 bytes

#define RXC_RH1M          0x0100  ///<  Rx header 1

//
//  Medium Status register
//

#define MS_FD             0x0002  ///<  Full duplex
#define MS_ONE            0x0004  ///<  Must be one
#define MS_RFC            0x0010  ///<  RX flow control enable
#define MS_TFC            0x0020  ///<  TX flow control enable
#define MS_PF             0x0080  ///<  Pause frame enable
#define MS_RE             0x0100  ///<  Receive enable
#define MS_PS             0x0200  ///<  Port speed 1=100, 0=10 Mbps
#define MS_SBP            0x0800  ///<  Stop back pressure
#define MS_SM             0x1000  ///<  Super MAC support

//
//  Software PHY Select register
//

#define SPHY_PSEL         (1 << 0)    ///<  Select internal PHY
#define SPHY_SSMII        (1 << 2)
#define SPHY_SSEN         (1 << 4)
#define SPHY_ASEL         0x02    ///<  1=Auto select, 0=Manual select

//
//  Software Reset register
//

#define SRR_RR            0x01    ///<  Clear receive frame length error
#define SRR_RT            0x02    ///<  Clear transmit frame length error
#define SRR_BZTYPE        0x04    ///<  External PHY reset pin tri-state enable
#define SRR_PRL           0x08    ///<  External PHY reset pin level
#define SRR_BZ            0x10    ///<  Force Bulk to return zero length packet
#define SRR_IPRL          0x20    ///<  Internal PHY reset control
#define SRR_IPPD          0x40    ///<  Internal PHY power down

//
//  PHY ID values
//

#define PHY_ID_INTERNAL   0x0001  ///<  Internal PHY

//
//  USB Commands
//

#define CMD_PHY_ACCESS_SOFTWARE   0x06  ///<  Software in control of PHY
#define CMD_PHY_REG_READ          0x07  ///<  Read PHY register, Value: PHY, Index: Register, Data: Register value
#define CMD_PHY_REG_WRITE         0x08  ///<  Write PHY register, Value: PHY, Index: Register, Data: New 16-bit value
#define CMD_PHY_ACCESS_HARDWARE   0x0a  ///<  Hardware in control of PHY
#define CMD_SROM_READ             0x0b  ///<  Read SROM register: Value: Address, Data: Value
#define CMD_SROM_WRITE            0x0c  ///<  Read SROM register: Value: Address, Data: Value
#define CMD_SROM_WRITE_EN         0x0d
#define CMD_SROM_WRITE_DIS        0x0e
#define CMD_RX_CONTROL_WRITE      0x10  ///<  Set the RX control register, Value: New value
#define CMD_GAPS_WRITE            0x12  ///<  Write the gaps register, Value: New value
#define CMD_MAC_ADDRESS_READ      0x13  ///<  Read the MAC address, Data: 6 byte MAC address
#define CMD_MAC_ADDRESS_WRITE     0x14  ///<  Set the MAC address, Data: New 6 byte MAC address
#define CMD_MULTICAST_HASH_READ   0x15  ///<  Read the multicast hash table
#define CMD_MULTICAST_HASH_WRITE  0x16  ///<  Write the multicast hash table, Data: New 8 byte value
#define CMD_MEDIUM_STATUS_READ    0x1a  ///<  Read medium status register, Data: Register value
#define CMD_MEDIUM_STATUS_WRITE   0x1b  ///<  Write medium status register, Value: New value
#define CMD_WRITE_GPIOS           0x1f
#define CMD_RESET                 0x20  ///<  Reset register, Value: New value
#define CMD_PHY_SELECT            0x22  ///<  PHY select register, Value: New value

#define CMD_RXQTC                 0x2a  ///<  RX Queue Cascade Threshold Control Register

//------------------------------
//  USB Endpoints
//------------------------------

#define CONTROL_ENDPOINT                0       ///<  Control endpoint
#define INTERRUPT_ENDPOINT              1       ///<  Interrupt endpoint
#define BULK_IN_ENDPOINT                2       ///<  Receive endpoint
#define BULK_OUT_ENDPOINT               3       ///<  Transmit endpoint

//------------------------------
//  PHY Registers
//------------------------------

#define PHY_BMCR                        0       ///<  Control register
#define PHY_BMSR                        1       ///<  Status register
#define PHY_ANAR                        4       ///<  Autonegotiation advertisement register
#define PHY_ANLPAR                      5       ///<  Autonegotiation link parter ability register
#define PHY_ANER                        6       ///<  Autonegotiation expansion register

//  BMCR - Register 0

#define BMCR_RESET                      0x8000  ///<  1 = Reset the PHY, bit clears after reset
#define BMCR_LOOPBACK                   0x4000  ///<  1 = Loopback enabled
#define BMCR_100MBPS                    0x2000  ///<  100 Mbits/Sec
#define BMCR_10MBPS                     0       ///<  10 Mbits/Sec
#define BMCR_AUTONEGOTIATION_ENABLE     0x1000  ///<  1 = Enable autonegotiation
#define BMCR_POWER_DOWN                 0x0800  ///<  1 = Power down
#define BMCR_ISOLATE                    0x0400  ///<  0 = Isolate PHY
#define BMCR_RESTART_AUTONEGOTIATION    0x0200  ///<  1 = Restart autonegotiation
#define BMCR_FULL_DUPLEX                0x0100  ///<  Full duplex operation
#define BMCR_HALF_DUPLEX                0       ///<  Half duplex operation
#define BMCR_COLLISION_TEST             0x0080  ///<  1 = Collision test enabled

//  BMSR - Register 1

#define BMSR_100BASET4                  0x8000  ///<  1 = 100BASE-T4 mode
#define BMSR_100BASETX_FDX              0x4000  ///<  1 = 100BASE-TX full duplex
#define BMSR_100BASETX_HDX              0x2000  ///<  1 = 100BASE-TX half duplex
#define BMSR_10BASET_FDX                0x1000  ///<  1 = 10BASE-T full duplex
#define BMSR_10BASET_HDX                0x0800  ///<  1 = 10BASE-T half duplex
#define BMSR_MF                         0x0040  ///<  1 = PHY accepts frames with preamble suppressed
#define BMSR_AUTONEG_CMPLT              0x0020  ///<  1 = Autonegotiation complete
#define BMSR_RF                         0x0010  ///<  1 = Remote fault
#define BMSR_AUTONEG                    0x0008  ///<  1 = Able to perform autonegotiation
#define BMSR_LINKST                     0x0004  ///<  1 = Link up
#define BMSR_JABBER_DETECT              0x0002  ///<  1 = jabber condition detected
#define BMSR_EXTENDED_CAPABILITY        0x0001  ///<  1 = Extended register capable

//  ANAR and ANLPAR Registers 4, 5

#define AN_NP                           0x8000  ///<  1 = Next page available
#define AN_ACK                          0x4000  ///<  1 = Link partner acknowledged
#define AN_RF                           0x2000  ///<  1 = Remote fault indicated by link partner
#define AN_FCS                          0x0400  ///<  1 = Flow control ability
#define AN_T4                           0x0200  ///<  1 = 100BASE-T4 support
#define AN_TX_FDX                       0x0100  ///<  1 = 100BASE-TX Full duplex
#define AN_TX_HDX                       0x0080  ///<  1 = 100BASE-TX support
#define AN_10_FDX                       0x0040  ///<  1 = 10BASE-T Full duplex
#define AN_10_HDX                       0x0020  ///<  1 = 10BASE-T support
#define AN_CSMA_CD                      0x0001  ///<  1 = IEEE 802.3 CSMA/CD support



//------------------------------------------------------------------------------
//  Data Types
//------------------------------------------------------------------------------

/**
  Ethernet header layout

  IEEE 802.3-2002 Part 3 specification, section 3.1.1.
**/
#pragma pack(1)
typedef struct {
  UINT8  DestAddr[PXE_HWADDR_LEN_ETHER];  ///<  Destination LAN address
  UINT8  SrcAddr[PXE_HWADDR_LEN_ETHER];   ///<  Source LAN address
  UINT16 Type;                            ///<  Protocol or length
} ETHERNET_HEADER;
#pragma pack()

/**
  Receive and Transmit packet structure
**/
#pragma pack(1)
typedef struct _TX_PACKET {
  UINT32  TxHdr1;
  UINT32  TxHdr2;
  UINT8   Data[USB_MAX_PKT_TX_SIZE];
} TX_PACKET;
#pragma pack()

#pragma pack(1)
typedef struct _RX_PACKET {
  UINT8             RxHdr1;
  UINT8             RxHdr2;
  UINT16            Length;
  UINT8             Data[USB_MAX_PKT_RX_SIZE];
} RX_PACKET;
#pragma pack()

/**
  Smsc95xx control structure

  The driver uses this structure to manage the MicroChip Smsc95xx 10/100
  Ethernet controller.
**/
typedef struct {
  UINTN                     Signature;         ///<  Structure identification

  //
  //  USB data
  //
  EFI_HANDLE                Controller;        ///<  Controller handle
  EFI_USB_IO_PROTOCOL       *UsbIo;            ///<  USB driver interface

  //
  //  Simple network protocol data
  //
  EFI_SIMPLE_NETWORK_PROTOCOL SimpleNetwork;     ///<  Driver's network stack interface
  EFI_SIMPLE_NETWORK_MODE     SimpleNetworkData; ///<  Data for simple network

  //
  // Ethernet controller data
  //
  BOOLEAN                   Initialized;       ///<  Controller initialized
  UINT16                    PhyId;             ///<  PHY ID

  //
  //  Link state
  //
  BOOLEAN                   LinkSpeed100Mbps;   ///<  Current link speed, FALSE = 10 Mbps
  BOOLEAN                   Complete;           ///<  Current state of auto-negotiation
  BOOLEAN                   FullDuplex;         ///<  Current duplex
  BOOLEAN                   LinkUp;             ///<  Current link state
  UINTN                     PollCount;          ///<  Number of times the autonegotiation status was polled

  //  接收数据相关
  RX_PACKET                 *BulkInbuf;
#ifdef TURBO_MODE
  UINT16                    BulkInbufIndex;
  UINT16                    BulkInbufLegth;
  UINT8                     RxBurst;
#endif

  // 发送数据相关
  TX_PACKET                 *BulkOutBuf;
  VOID                      *TxBuffer;          // 记录发送数据缓存区的地址，等待回收

  UINT8                     MulticastHash[8];
  UINT32                    MacCR;

  UINT16                    CurRxControl;

  EFI_DEVICE_PATH_PROTOCOL  *MyDevPath;
  BOOLEAN                   FirstRst;

  UINT8                     BulkInEndpoint;
  UINT8                     BulkOutEndpoint;
  UINT8                     InterruptEndpoint;

} NIC_DEVICE;

#define DEV_FROM_SIMPLE_NETWORK(a)  CR (a, NIC_DEVICE, SimpleNetwork, DEV_SIGNATURE)  ///< Locate NIC_DEVICE from Simple Network Protocol

//------------------------------------------------------------------------------
// Simple Network Protocol
//------------------------------------------------------------------------------

/**
  Reset the network adapter.

  Resets a network adapter and reinitializes it with the parameters that
  were provided in the previous call to Initialize ().  The transmit and
  receive queues are cleared.  Receive filters, the station address, the
  statistics, and the multicast-IP-to-HW MAC addresses are not reset by
  this call.

  This routine calls ::Smsc95xxReset to perform the adapter specific
  reset operation.  This routine also starts the link negotiation
  by calling ::Smsc95xxNegotiateLinkStart.

  @param [in] SimpleNetwork    Protocol instance pointer
  @param [in] ExtendedVerification  Indicates that the driver may perform a more
                                exhaustive verification operation of the device
                                during reset.

  @retval EFI_SUCCESS           This operation was successful.
  @retval EFI_NOT_STARTED       The network interface was not started.
  @retval EFI_INVALID_PARAMETER SimpleNetwork parameter was NULL or did not point to a valid
                                EFI_SIMPLE_NETWORK_PROTOCOL structure.
  @retval EFI_DEVICE_ERROR      The command could not be sent to the network interface.
  @retval EFI_UNSUPPORTED       The increased buffer size feature is not supported.

**/
EFI_STATUS
EFIAPI
SN_Reset (
  IN EFI_SIMPLE_NETWORK_PROTOCOL *SimpleNetwork,
  IN BOOLEAN                     ExtendedVerification
  );

/**
  Initialize the simple network protocol.

  This routine calls ::Smsc95xxMacAddressGet to obtain the
  MAC address.

  @param [in] NicDevice       NIC_DEVICE_INSTANCE pointer

  @retval EFI_SUCCESS     Setup was successful

**/
EFI_STATUS
SN_Setup (
  IN NIC_DEVICE *NicDevice
  );

/**
  This routine starts the network interface.

  @param [in] SimpleNetwork    Protocol instance pointer

  @retval EFI_SUCCESS           This operation was successful.
  @retval EFI_ALREADY_STARTED   The network interface was already started.
  @retval EFI_INVALID_PARAMETER SimpleNetwork parameter was NULL or did not point to a valid
                                EFI_SIMPLE_NETWORK_PROTOCOL structure.
  @retval EFI_DEVICE_ERROR      The command could not be sent to the network interface.
  @retval EFI_UNSUPPORTED       The increased buffer size feature is not supported.

**/
EFI_STATUS
EFIAPI
SN_Start (
  IN EFI_SIMPLE_NETWORK_PROTOCOL *SimpleNetwork
  );

/**
  Set the MAC address.

  This function modifies or resets the current station address of a
  network interface.  If Reset is TRUE, then the current station address
  is set ot the network interface's permanent address.  If Reset if FALSE
  then the current station address is changed to the address specified by
  New.

  This routine calls ::Smsc95xxMacAddressSet to update the MAC address
  in the network adapter.

  @param [in] SimpleNetwork    Protocol instance pointer
  @param [in] Reset            Flag used to reset the station address to the
                                network interface's permanent address.
  @param [in] New              New station address to be used for the network
                                interface.

  @retval EFI_SUCCESS           This operation was successful.
  @retval EFI_NOT_STARTED       The network interface was not started.
  @retval EFI_INVALID_PARAMETER SimpleNetwork parameter was NULL or did not point to a valid
                                EFI_SIMPLE_NETWORK_PROTOCOL structure.
  @retval EFI_DEVICE_ERROR      The command could not be sent to the network interface.
  @retval EFI_UNSUPPORTED       The increased buffer size feature is not supported.

**/
EFI_STATUS
EFIAPI
SN_StationAddress (
  IN EFI_SIMPLE_NETWORK_PROTOCOL *SimpleNetwork,
  IN BOOLEAN                     Reset,
  IN EFI_MAC_ADDRESS             *New
  );

/**
  This function resets or collects the statistics on a network interface.
  If the size of the statistics table specified by StatisticsSize is not
  big enough for all of the statistics that are collected by the network
  interface, then a partial buffer of statistics is returned in
  StatisticsTable.

  @param [in] SimpleNetwork    Protocol instance pointer
  @param [in] Reset            Set to TRUE to reset the statistics for the network interface.
  @param [in, out] StatisticsSize  On input the size, in bytes, of StatisticsTable.  On output
                                the size, in bytes, of the resulting table of statistics.
  @param [out] StatisticsTable A pointer to the EFI_NETWORK_STATISTICS structure that
                                conains the statistics.

  @retval EFI_SUCCESS           This operation was successful.
  @retval EFI_NOT_STARTED       The network interface was not started.
  @retval EFI_BUFFER_TOO_SMALL  The StatisticsTable is NULL or the buffer is too small.
  @retval EFI_INVALID_PARAMETER SimpleNetwork parameter was NULL or did not point to a valid
                                EFI_SIMPLE_NETWORK_PROTOCOL structure.
  @retval EFI_DEVICE_ERROR      The command could not be sent to the network interface.
  @retval EFI_UNSUPPORTED       The increased buffer size feature is not supported.

**/
EFI_STATUS
EFIAPI
SN_Statistics (
  IN     EFI_SIMPLE_NETWORK_PROTOCOL *SimpleNetwork,
  IN     BOOLEAN                     Reset,
  IN OUT UINTN                       *StatisticsSize,
  OUT    EFI_NETWORK_STATISTICS      *StatisticsTable
  );

/**
  This function stops a network interface.  This call is only valid
  if the network interface is in the started state.

  @param [in] SimpleNetwork    Protocol instance pointer

  @retval EFI_SUCCESS           This operation was successful.
  @retval EFI_NOT_STARTED       The network interface was not started.
  @retval EFI_INVALID_PARAMETER SimpleNetwork parameter was NULL or did not point to a valid
                                EFI_SIMPLE_NETWORK_PROTOCOL structure.
  @retval EFI_DEVICE_ERROR      The command could not be sent to the network interface.
  @retval EFI_UNSUPPORTED       The increased buffer size feature is not supported.

**/
EFI_STATUS
EFIAPI
SN_Stop (
  IN EFI_SIMPLE_NETWORK_PROTOCOL *SimpleNetwork
  );

/**
  This function releases the memory buffers assigned in the Initialize() call.
  Ending transmits and receives are lost, and interrupts are cleared and disabled.
  After this call, only Initialize() and Stop() calls may be used.

  @param [in] SimpleNetwork    Protocol instance pointer

  @retval EFI_SUCCESS           This operation was successful.
  @retval EFI_NOT_STARTED       The network interface was not started.
  @retval EFI_INVALID_PARAMETER SimpleNetwork parameter was NULL or did not point to a valid
                                EFI_SIMPLE_NETWORK_PROTOCOL structure.
  @retval EFI_DEVICE_ERROR      The command could not be sent to the network interface.
  @retval EFI_UNSUPPORTED       The increased buffer size feature is not supported.

**/
EFI_STATUS
EFIAPI
SN_Shutdown (
  IN EFI_SIMPLE_NETWORK_PROTOCOL *SimpleNetwork
  );

/**
  Send a packet over the network.

  This function places the packet specified by Header and Buffer on
  the transmit queue.  This function performs a non-blocking transmit
  operation.  When the transmit is complete, the buffer is returned
  via the GetStatus() call.

  This routine calls ::Smsc95xxRx to empty the network adapter of
  receive packets.  The routine then passes the transmit packet
  to the network adapter.

  @param [in] SimpleNetwork    Protocol instance pointer
  @param [in] HeaderSize        The size, in bytes, of the media header to be filled in by
                                the Transmit() function.  If HeaderSize is non-zero, then
                                it must be equal to SimpleNetwork->Mode->MediaHeaderSize
                                and DestAddr and Protocol parameters must not be NULL.
  @param [in] BufferSize        The size, in bytes, of the entire packet (media header and
                                data) to be transmitted through the network interface.
  @param [in] Buffer           A pointer to the packet (media header followed by data) to
                                to be transmitted.  This parameter can not be NULL.  If
                                HeaderSize is zero, then the media header is Buffer must
                                already be filled in by the caller.  If HeaderSize is nonzero,
                                then the media header will be filled in by the Transmit()
                                function.
  @param [in] SrcAddr          The source HW MAC address.  If HeaderSize is zero, then
                                this parameter is ignored.  If HeaderSize is nonzero and
                                SrcAddr is NULL, then SimpleNetwork->Mode->CurrentAddress
                                is used for the source HW MAC address.
  @param [in] DestAddr         The destination HW MAC address.  If HeaderSize is zero, then
                                this parameter is ignored.
  @param [in] Protocol         The type of header to build.  If HeaderSize is zero, then
                                this parameter is ignored.

  @retval EFI_SUCCESS           This operation was successful.
  @retval EFI_NOT_STARTED       The network interface was not started.
  @retval EFI_NOT_READY         The network interface is too busy to accept this transmit request.
  @retval EFI_BUFFER_TOO_SMALL  The BufferSize parameter is too small.
  @retval EFI_INVALID_PARAMETER SimpleNetwork parameter was NULL or did not point to a valid
                                EFI_SIMPLE_NETWORK_PROTOCOL structure.
  @retval EFI_DEVICE_ERROR      The command could not be sent to the network interface.

**/
EFI_STATUS
EFIAPI
SN_Transmit (
  IN EFI_SIMPLE_NETWORK_PROTOCOL *SimpleNetwork,
  IN UINTN           HeaderSize,
  IN UINTN           BufferSize,
  IN VOID            *Buffer,
  IN EFI_MAC_ADDRESS *SrcAddr,
  IN EFI_MAC_ADDRESS *DestAddr,
  IN UINT16          *Protocol
  );

//------------------------------------------------------------------------------
// Support Routines
//------------------------------------------------------------------------------

EFI_STATUS
Smsc95xxMacAddressInit (
  OUT UINT8     *MacAddress
  );

/**
  Get the MAC address

  This routine calls ::Smsc95xxUsbCommand to request the MAC
  address from the network adapter.

  @param [in] NicDevice       Pointer to the NIC_DEVICE structure
  @param [out] MacAddress      Address of a six byte buffer to receive the MAC address.

  @retval EFI_SUCCESS          The MAC address is available.
  @retval other                The MAC address is not valid.

**/
EFI_STATUS
Smsc95xxMacAddressGet (
  IN NIC_DEVICE *NicDevice,
  OUT UINT8     *MacAddress
  );

/**
  Set the MAC address

  This routine calls ::Smsc95xxUsbCommand to set the MAC address
  in the network adapter.

  @param [in] NicDevice       Pointer to the NIC_DEVICE structure
  @param [in] MacAddress      Address of a six byte buffer to containing the new MAC address.

  @retval EFI_SUCCESS          The MAC address was set.
  @retval other                The MAC address was not set.

**/
EFI_STATUS
Smsc95xxMacAddressSet (
  IN NIC_DEVICE   *NicDevice,
  IN CONST UINT8  *MacAddress
  );

/**
  Clear the multicast hash table

  @param [in] NicDevice       Pointer to the NIC_DEVICE structure

**/
VOID
Smsc95xxMulticastClear (
  IN NIC_DEVICE *NicDevice
  );

/**
  Enable a multicast address in the multicast hash table

  This routine calls ::Smsc95xxCrc to compute the hash bit for
  this MAC address.

  @param [in] NicDevice       Pointer to the NIC_DEVICE structure
  @param [in] MacAddress      Address of a six byte buffer to containing the MAC address.

**/
VOID
Smsc95xxMulticastSet (
  IN NIC_DEVICE *NicDevice,
  IN UINT8      *MacAddress
  );

/**
  Start the link negotiation

  This routine calls ::Smsc95xxPhyWrite to start the PHY's link
  negotiation.

  @param [in] NicDevice       Pointer to the NIC_DEVICE structure

  @retval EFI_SUCCESS          The link negotiation was started.
  @retval other                Failed to start the link negotiation.

**/
EFI_STATUS
Smsc95xxNegotiateLinkStart (
  IN NIC_DEVICE *NicDevice
  );

/**
  Complete the negotiation of the PHY link

  This routine calls ::Smsc95xxPhyRead to determine if the
  link negotiation is complete.

  @param [in] NicDevice       Pointer to the NIC_DEVICE structure
  @param [in, out] PollCount  Address of number of times this routine was polled
  @param [out] Complete      Address of boolean to receive complate status.
  @param [out] LinkUp        Address of boolean to receive link status, TRUE=up.
  @param [out] HiSpeed       Address of boolean to receive link speed, TRUE=100Mbps.
  @param [out] FullDuplex    Address of boolean to receive link duplex, TRUE=full.

  @retval EFI_SUCCESS          The MAC address is available.
  @retval other                The MAC address is not valid.

**/
EFI_STATUS
Smsc95xxNegotiateLinkComplete (
  IN     NIC_DEVICE *NicDevice,
  IN OUT UINTN      *PollCount,
  OUT    BOOLEAN    *Complete,
  OUT    BOOLEAN    *LinkUp,
  OUT    BOOLEAN    *HiSpeed,
  OUT    BOOLEAN    *FullDuplex
  );

/**
  Read a register from the PHY

  This routine calls ::Smsc95xxUsbCommand to read a PHY register.

  @param [in] NicDevice       Pointer to the NIC_DEVICE structure
  @param [in] RegisterAddress  Number of the register to read.
  @param [in, out] PhyData    Address of a buffer to receive the PHY register value

  @retval EFI_SUCCESS          The PHY data is available.
  @retval other                The PHY data is not valid.

**/


/**
  Reset the Smsc95xx

  This routine uses ::Smsc95xxUsbCommand to reset the network
  adapter.  This routine also uses ::Smsc95xxPhyWrite to reset
  the PHY.

  @param [in] NicDevice       Pointer to the NIC_DEVICE structure

  @retval EFI_SUCCESS          The MAC address is available.
  @retval other                The MAC address is not valid.

**/
EFI_STATUS
Smsc95xxReset (
  IN NIC_DEVICE *NicDevice
  );

/**
  Enable or disable the receiver

  This routine calls ::Smsc95xxUsbCommand to update the
  receiver state.  This routine also calls ::Smsc95xxMacAddressSet
  to establish the MAC address for the network adapter.

  @param [in] NicDevice       Pointer to the NIC_DEVICE structure
  @param [in] RxFilter         Simple network RX filter mask value

  @retval EFI_SUCCESS          The MAC address was set.
  @retval other                The MAC address was not set.

**/
EFI_STATUS
Smsc95xxRxControl (
  IN NIC_DEVICE *NicDevice,
  IN UINT32     RxFilter
  );

EFI_STATUS
Smsc95xxReloadSrom  (
  IN NIC_DEVICE *NicDevice
  );

/**
  Read an SROM location

  This routine calls ::Smsc95xxUsbCommand to read data from the
  SROM.

  @param [in] NicDevice       Pointer to the NIC_DEVICE structure
  @param [in] Address          SROM address
  @param [out] Data           Buffer to receive the data

  @retval EFI_SUCCESS          The read was successful
  @retval other                The read failed

**/
EFI_STATUS
Smsc95xxSromRead (
  IN  NIC_DEVICE *NicDevice,
  IN  UINT32     Address,
  OUT UINT16     *Data
  );


EFI_STATUS
Smsc95xxEnableSromWrite  (
  IN NIC_DEVICE *NicDevice
  );


EFI_STATUS
Smsc95xxDisableSromWrite  (
  IN NIC_DEVICE *NicDevice
  );

EFI_STATUS
Smsc95xxSromWrite (
  IN  NIC_DEVICE *NicDevice,
  IN  UINT32     Address,
  OUT UINT16     *Data
  );

/**
  Send a command to the USB device.

  @param [in] NicDevice       Pointer to the NIC_DEVICE structure
  @param [in] Request         Pointer to the request structure
  @param [in, out] Buffer     Data buffer address

  @retval EFI_SUCCESS          The USB transfer was successful
  @retval other                The USB transfer failed

**/

EFI_STATUS
Smsc95xxUsbCommand (
  IN NIC_DEVICE         *NicDevice,
  IN USB_DEVICE_REQUEST *Request,
  IN OUT VOID           *Buffer
  );

//------------------------------------------------------------------------------
// EFI Component Name Protocol Support
//------------------------------------------------------------------------------
extern EFI_DRIVER_BINDING_PROTOCOL   gDriverBinding;
extern EFI_COMPONENT_NAME_PROTOCOL   gComponentName;  ///<  Component name protocol declaration
extern EFI_COMPONENT_NAME2_PROTOCOL  gComponentName2; ///<  Component name 2 protocol declaration

/**
  Retrieves a Unicode string that is the user readable name of the driver.

  This function retrieves the user readable name of a driver in the form of a
  Unicode string. If the driver specified by This has a user readable name in
  the language specified by Language, then a pointer to the driver name is
  returned in DriverName, and EFI_SUCCESS is returned. If the driver specified
  by This does not support the language specified by Language,
  then EFI_UNSUPPORTED is returned.

  @param [in] This             A pointer to the EFI_COMPONENT_NAME2_PROTOCOL or
                                EFI_COMPONENT_NAME_PROTOCOL instance.
  @param [in] Language         A pointer to a Null-terminated ASCII string
                                array indicating the language. This is the
                                language of the driver name that the caller is
                                requesting, and it must match one of the
                                languages specified in SupportedLanguages. The
                                number of languages supported by a driver is up
                                to the driver writer. Language is specified
                                in RFC 3066 or ISO 639-2 language code format.
  @param [out] DriverName     A pointer to the Unicode string to return.
                                This Unicode string is the name of the
                                driver specified by This in the language
                                specified by Language.

  @retval EFI_SUCCESS           The Unicode string for the Driver specified by
                                This and the language specified by Language was
                                returned in DriverName.
  @retval EFI_INVALID_PARAMETER Language is NULL.
  @retval EFI_INVALID_PARAMETER DriverName is NULL.
  @retval EFI_UNSUPPORTED       The driver specified by This does not support
                                the language specified by Language.

**/
EFI_STATUS
EFIAPI
GetDriverName (
  IN  EFI_COMPONENT_NAME_PROTOCOL *This,
  IN  CHAR8                       *Language,
  OUT CHAR16                      **DriverName
  );


/**
  Retrieves a Unicode string that is the user readable name of the controller
  that is being managed by a driver.

  This function retrieves the user readable name of the controller specified by
  ControllerHandle and ChildHandle in the form of a Unicode string. If the
  driver specified by This has a user readable name in the language specified by
  Language, then a pointer to the controller name is returned in ControllerName,
  and EFI_SUCCESS is returned.  If the driver specified by This is not currently
  managing the controller specified by ControllerHandle and ChildHandle,
  then EFI_UNSUPPORTED is returned.  If the driver specified by This does not
  support the language specified by Language, then EFI_UNSUPPORTED is returned.

  @param [in] This             A pointer to the EFI_COMPONENT_NAME2_PROTOCOL or
                                EFI_COMPONENT_NAME_PROTOCOL instance.
  @param [in] ControllerHandle  The handle of a controller that the driver
                                specified by This is managing.  This handle
                                specifies the controller whose name is to be
                                returned.
  @param [in] ChildHandle       The handle of the child controller to retrieve
                                the name of.  This is an optional parameter that
                                may be NULL.  It will be NULL for device
                                drivers.  It will also be NULL for a bus drivers
                                that wish to retrieve the name of the bus
                                controller.  It will not be NULL for a bus
                                driver that wishes to retrieve the name of a
                                child controller.
  @param [in] Language         A pointer to a Null-terminated ASCII string
                                array indicating the language.  This is the
                                language of the driver name that the caller is
                                requesting, and it must match one of the
                                languages specified in SupportedLanguages. The
                                number of languages supported by a driver is up
                                to the driver writer. Language is specified in
                                RFC 3066 or ISO 639-2 language code format.
  @param [out] ControllerName A pointer to the Unicode string to return.
                                This Unicode string is the name of the
                                controller specified by ControllerHandle and
                                ChildHandle in the language specified by
                                Language from the point of view of the driver
                                specified by This.

  @retval EFI_SUCCESS           The Unicode string for the user readable name in
                                the language specified by Language for the
                                driver specified by This was returned in
                                DriverName.
  @retval EFI_INVALID_PARAMETER ControllerHandle is not a valid EFI_HANDLE.
  @retval EFI_INVALID_PARAMETER ChildHandle is not NULL and it is not a valid
                                EFI_HANDLE.
  @retval EFI_INVALID_PARAMETER Language is NULL.
  @retval EFI_INVALID_PARAMETER ControllerName is NULL.
  @retval EFI_UNSUPPORTED       The driver specified by This is not currently
                                managing the controller specified by
                                ControllerHandle and ChildHandle.
  @retval EFI_UNSUPPORTED       The driver specified by This does not support
                                the language specified by Language.

**/
EFI_STATUS
EFIAPI
GetControllerName (
  IN  EFI_COMPONENT_NAME_PROTOCOL *This,
  IN  EFI_HANDLE                  ControllerHandle,
  IN OPTIONAL EFI_HANDLE          ChildHandle,
  IN  CHAR8                       *Language,
  OUT CHAR16                      **ControllerName
  );


/* Tx command words */
#define TX_CMD_A_DATA_OFFSET_  (0x001F0000) /* Data Start Offset */
#define TX_CMD_A_FIRST_SEG_    (0x00002000) /* First Segment */
#define TX_CMD_A_LAST_SEG_     (0x00001000) /* Last Segment */
#define TX_CMD_A_BUF_SIZE_     (0x000007FF) /* Buffer Size */

#define TX_CMD_B_CSUM_ENABLE   (0x00004000) /* TX Checksum Enable */
#define TX_CMD_B_ADD_CRC_DIS_  (0x00002000) /* Add CRC Disable */
#define TX_CMD_B_DIS_PADDING_  (0x00001000) /* Disable Frame Padding */
#define TX_CMD_B_FRAME_LENGTH_ (0x000007FF) /* Frame Length (bytes) */

/* Rx status word */
#define RX_STS_FF_  (0x40000000) /* Filter Fail */
#define RX_STS_FL_  (0x3FFF0000) /* Frame Length */
#define RX_STS_ES_  (0x00008000) /* Error Summary */
#define RX_STS_BF_  (0x00002000) /* Broadcast Frame */
#define RX_STS_LE_  (0x00001000) /* Length Error */
#define RX_STS_RF_  (0x00000800) /* Runt Frame */
#define RX_STS_MF_  (0x00000400) /* Multicast Frame */
#define RX_STS_TL_  (0x00000080) /* Frame too long */
#define RX_STS_CS_  (0x00000040) /* Collision Seen */
#define RX_STS_FT_  (0x00000020) /* Frame Type */
#define RX_STS_RW_  (0x00000010) /* Receive Watchdog */
#define RX_STS_ME_  (0x00000008) /* MII Error */
#define RX_STS_DB_  (0x00000004) /* Dribbling */
#define RX_STS_CRC_ (0x00000002) /* CRC Error */

/* SCSRs - System Control and Status Registers */
/* Device ID and Revision Register */
#define ID_REV (0x00)
  #define ID_REV_CHIP_ID_MASK_  (0xFFFF0000)
  #define ID_REV_CHIP_REV_MASK_ (0x0000FFFF)
  #define ID_REV_CHIP_ID_9500_  (0x9500)
  #define ID_REV_CHIP_ID_9500A_ (0x9E00)
  #define ID_REV_CHIP_ID_9512_  (0xEC00)
  #define ID_REV_CHIP_ID_9530_  (0x9530)
  #define ID_REV_CHIP_ID_89530_ (0x9E08)
  #define ID_REV_CHIP_ID_9730_  (0x9730)

/* Interrupt Status Register */
#define INT_STS (0x08)
  #define INT_STS_MAC_RTO_   (0x00040000) /* MAC Reset Time Out */
  #define INT_STS_TX_STOP_   (0x00020000) /* TX Stopped */
  #define INT_STS_RX_STOP_   (0x00010000) /* RX Stopped */
  #define INT_STS_PHY_INT_   (0x00008000) /* PHY Interrupt */
  #define INT_STS_TXE_       (0x00004000) /* Transmitter Error */
  #define INT_STS_TDFU_      (0x00002000) /* TX Data FIFO Underrun */
  #define INT_STS_TDFO_      (0x00001000) /* TX Data FIFO Overrun */
  #define INT_STS_RXDF_      (0x00000800) /* RX Dropped Frame */
  #define INT_STS_GPIOS_     (0x000007FF) /* GPIOs Interrupts */
  #define INT_STS_CLEAR_ALL_ (0xFFFFFFFF)

/* Receive Configuration Register */
#define RX_CFG (0x0C)
  #define RX_FIFO_FLUSH_ (0x00000001) /* Receive FIFO Flush */

/* Transmit Configuration Register */
#define TX_CFG (0x10)
  #define TX_CFG_ON_         (0x00000004) /* Transmitter Enable */
  #define TX_CFG_STOP_       (0x00000002) /* Stop Transmitter */
  #define TX_CFG_FIFO_FLUSH_ (0x00000001) /* Transmit FIFO Flush */

/* Hardware Configuration Register */
#define HW_CFG (0x14)
  #define HW_CFG_BIR_    (0x00001000) /* Bulk In Empty Response */
  #define HW_CFG_LEDB_   (0x00000800) /* Activity LED 80ms Bypass */
  #define HW_CFG_RXDOFF_ (0x00000600) /* RX Data Offset */
  #define HW_CFG_SBP_    (0x00000100) /* Stall Bulk Out Pipe Dis. */
  #define HW_CFG_IME_    (0x00000080) /* Internal MII Visi. Enable */
  #define HW_CFG_DRP_    (0x00000040) /* Discard Errored RX Frame */
  #define HW_CFG_MEF_    (0x00000020) /* Mult. ETH Frames/USB pkt */
  #define HW_CFG_ETC_    (0x00000010) /* EEPROM Timeout Control */
  #define HW_CFG_LRST_   (0x00000008) /* Soft Lite Reset */
  #define HW_CFG_PSEL_   (0x00000004) /* External PHY Select */
  #define HW_CFG_BCE_    (0x00000002) /* Burst Cap Enable */
  #define HW_CFG_SRST_   (0x00000001) /* Soft Reset */

/* Receive FIFO Information Register */
#define RX_FIFO_INF (0x18)
  #define RX_FIFO_INF_USED_ (0x0000FFFF) /* RX Data FIFO Used Space */

/* Transmit FIFO Information Register */
#define TX_FIFO_INF (0x1C)
  #define TX_FIFO_INF_FREE_ (0x0000FFFF) /* TX Data FIFO Free Space */

/* Power Management Control Register */
#define PM_CTRL (0x20)
  #define PM_CTL_RES_CLR_WKP_STS (0x00000200) /* Resume Clears Wakeup STS */
  #define PM_CTL_RES_CLR_WKP_EN  (0x00000100) /* Resume Clears Wkp Enables */
  #define PM_CTL_DEV_RDY_        (0x00000080) /* Device Ready */
  #define PM_CTL_SUS_MODE_       (0x00000060) /* Suspend Mode */
  #define PM_CTL_SUS_MODE_0      (0x00000000)
  #define PM_CTL_SUS_MODE_1      (0x00000020)
  #define PM_CTL_SUS_MODE_2      (0x00000040)
  #define PM_CTL_SUS_MODE_3      (0x00000060)
  #define PM_CTL_PHY_RST_        (0x00000010) /* PHY Reset */
  #define PM_CTL_WOL_EN_         (0x00000008) /* Wake On Lan Enable */
  #define PM_CTL_ED_EN_          (0x00000004) /* Energy Detect Enable */
  #define PM_CTL_WUPS_           (0x00000003) /* Wake Up Status */
  #define PM_CTL_WUPS_NO_        (0x00000000) /* No Wake Up Event Detected */
  #define PM_CTL_WUPS_ED_        (0x00000001) /* Energy Detect */
  #define PM_CTL_WUPS_WOL_       (0x00000002) /* Wake On Lan */
  #define PM_CTL_WUPS_MULTI_     (0x00000003) /* Multiple Events Occurred */

/* LED General Purpose IO Configuration Register */
#define LED_GPIO_CFG (0x24)
  #define LED_GPIO_CFG_SPD_LED (0x01000000) /* GPIOz as Speed LED */
  #define LED_GPIO_CFG_LNK_LED (0x00100000) /* GPIOy as Link LED */
  #define LED_GPIO_CFG_FDX_LED (0x00010000) /* GPIOx as Full Duplex LED */

/* General Purpose IO Configuration Register */
#define GPIO_CFG (0x28)

/* Automatic Flow Control Configuration Register */
#define AFC_CFG (0x2C)
  #define AFC_CFG_HI_       (0x00FF0000) /* Auto Flow Ctrl High Level */
  #define AFC_CFG_LO_       (0x0000FF00) /* Auto Flow Ctrl Low Level */
  #define AFC_CFG_BACK_DUR_ (0x000000F0) /* Back Pressure Duration */
  #define AFC_CFG_FC_MULT_  (0x00000008) /* Flow Ctrl on Mcast Frame */
  #define AFC_CFG_FC_BRD_   (0x00000004) /* Flow Ctrl on Bcast Frame */
  #define AFC_CFG_FC_ADD_   (0x00000002) /* Flow Ctrl on Addr. Decode */
  #define AFC_CFG_FC_ANY_   (0x00000001) /* Flow Ctrl on Any Frame */
  /* Hi watermark = 15.5Kb (~10 mtu pkts) */
  /* low watermark = 3k (~2 mtu pkts) */
  /* backpressure duration = ~ 350us */
  /* Apply FC on any frame. */
  #define AFC_CFG_DEFAULT   (0x00F830A1)

/* EEPROM Command Register */
#define E2P_CMD (0x30)
  #define E2P_CMD_BUSY_    (0x80000000) /* E2P Controller Busy */
  #define E2P_CMD_MASK_    (0x70000000) /* Command Mask (see below) */
  #define E2P_CMD_READ_    (0x00000000) /* Read Location */
  #define E2P_CMD_EWDS_    (0x10000000) /* Erase/Write Disable */
  #define E2P_CMD_EWEN_    (0x20000000) /* Erase/Write Enable */
  #define E2P_CMD_WRITE_   (0x30000000) /* Write Location */
  #define E2P_CMD_WRAL_    (0x40000000) /* Write All */
  #define E2P_CMD_ERASE_   (0x50000000) /* Erase Location */
  #define E2P_CMD_ERAL_    (0x60000000) /* Erase All */
  #define E2P_CMD_RELOAD_  (0x70000000) /* Data Reload */
  #define E2P_CMD_TIMEOUT_ (0x00000400) /* Set if no resp within 30ms */
  #define E2P_CMD_LOADED_  (0x00000200) /* Valid EEPROM found */
  #define E2P_CMD_ADDR_    (0x000001FF) /* Byte aligned address */

#define MAX_EEPROM_SIZE (512)

/* EEPROM Data Register */
#define E2P_DATA (0x34)
  #define E2P_DATA_MASK_ (0x000000FF) /* EEPROM Data Mask */

/* Burst Cap Register */
#define BURST_CAP (0x38)
  #define BURST_CAP_MASK_ (0x000000FF) /* Max burst sent by the UTX */

/* Configuration Straps Status Register */
#define STRAP_STATUS (0x3C)
  #define STRAP_STATUS_PWR_SEL_     (0x00000020) /* Device self-powered */
  #define STRAP_STATUS_AMDIX_EN_    (0x00000010) /* Auto-MDIX Enabled */
  #define STRAP_STATUS_PORT_SWAP_   (0x00000008) /* USBD+/USBD- Swapped */
  #define STRAP_STATUS_EEP_SIZE_    (0x00000004) /* EEPROM Size */
  #define STRAP_STATUS_RMT_WKP_     (0x00000002) /* Remote Wkp supported */
  #define STRAP_STATUS_EEP_DISABLE_ (0x00000001) /* EEPROM Disabled */

/* Data Port Select Register */
#define DP_SEL (0x40)

/* Data Port Command Register */
#define DP_CMD (0x44)

/* Data Port Address Register */
#define DP_ADDR (0x48)

/* Data Port Data 0 Register */
#define DP_DATA0 (0x4C)

/* Data Port Data 1 Register */
#define DP_DATA1 (0x50)

/* General Purpose IO Wake Enable and Polarity Register */
#define GPIO_WAKE (0x64)

/* Interrupt Endpoint Control Register */
#define INT_EP_CTL (0x68)
  #define INT_EP_CTL_INTEP_   (0x80000000) /* Always TX Interrupt PKT */
  #define INT_EP_CTL_MAC_RTO_ (0x00080000) /* MAC Reset Time Out */
  #define INT_EP_CTL_RX_FIFO_ (0x00040000) /* RX FIFO Has Frame */
  #define INT_EP_CTL_TX_STOP_ (0x00020000) /* TX Stopped */
  #define INT_EP_CTL_RX_STOP_ (0x00010000) /* RX Stopped */
  #define INT_EP_CTL_PHY_INT_ (0x00008000) /* PHY Interrupt */
  #define INT_EP_CTL_TXE_     (0x00004000) /* TX Error */
  #define INT_EP_CTL_TDFU_    (0x00002000) /* TX Data FIFO Underrun */
  #define INT_EP_CTL_TDFO_    (0x00001000) /* TX Data FIFO Overrun */
  #define INT_EP_CTL_RXDF_    (0x00000800) /* RX Dropped Frame */
  #define INT_EP_CTL_GPIOS_   (0x000007FF) /* GPIOs Interrupt Enable */

/* Bulk In Delay Register (units of 16.667ns, until ~1092µs) */
#define BULK_IN_DLY (0x6C)

/* MAC CSRs - MAC Control and Status Registers */
/* MAC Control Register */
#define MAC_CR (0x100)
  #define MAC_CR_RXALL_     (0x80000000) /* Receive All Mode */
  #define MAC_CR_RCVOWN_    (0x00800000) /* Disable Receive Own */
  #define MAC_CR_LOOPBK_    (0x00200000) /* Loopback Operation Mode */
  #define MAC_CR_FDPX_      (0x00100000) /* Full Duplex Mode */
  #define MAC_CR_MCPAS_     (0x00080000) /* Pass All Multicast */
  #define MAC_CR_PRMS_      (0x00040000) /* Promiscuous Mode */
  #define MAC_CR_INVFILT_   (0x00020000) /* Inverse Filtering */
  #define MAC_CR_PASSBAD_   (0x00010000) /* Pass Bad Frames */
  #define MAC_CR_HFILT_     (0x00008000) /* Hash Only Filtering Mode */
  #define MAC_CR_HPFILT_    (0x00002000) /* Hash/Perfect Filt. Mode */
  #define MAC_CR_LCOLL_     (0x00001000) /* Late Collision Control */
  #define MAC_CR_BCAST_     (0x00000800) /* Disable Broadcast Frames */
  #define MAC_CR_DISRTY_    (0x00000400) /* Disable Retry */
  #define MAC_CR_PADSTR_    (0x00000100) /* Automatic Pad Stripping */
  #define MAC_CR_BOLMT_MASK (0x000000C0) /* BackOff Limit */
  #define MAC_CR_DFCHK_     (0x00000020) /* Deferral Check */
  #define MAC_CR_TXEN_      (0x00000008) /* Transmitter Enable */
  #define MAC_CR_RXEN_      (0x00000004) /* Receiver Enable */

/* MAC Address High Register */
#define ADDRH (0x104)

/* MAC Address Low Register */
#define ADDRL (0x108)

/* Multicast Hash Table High Register */
#define HASHH (0x10C)

/* Multicast Hash Table Low Register */
#define HASHL (0x110)

/* MII Access Register */
#define MII_ADDR   (0x114)
  #define MII_WRITE_ (0x02)
  #define MII_BUSY_  (0x01)
  #define MII_READ_  (0x00) /* ~of MII Write bit */

/* MII Data Register */
#define MII_DATA (0x118)

/* Flow Control Register */
#define FLOW (0x11C)
  #define FLOW_FCPT_   (0xFFFF0000) /* Pause Time */
  #define FLOW_FCPASS_ (0x00000004) /* Pass Control Frames */
  #define FLOW_FCEN_   (0x00000002) /* Flow Control Enable */
  #define FLOW_FCBSY_  (0x00000001) /* Flow Control Busy */

/* VLAN1 Tag Register */
#define VLAN1 (0x120)

/* VLAN2 Tag Register */
#define VLAN2 (0x124)

/* Wake Up Frame Filter Register */
#define WUFF (0x128)
  #define LAN9500_WUFF_NUM (4)
  #define LAN9500A_WUFF_NUM (8)

/* Wake Up Control and Status Register */
#define WUCSR (0x12C)
  #define WUCSR_WFF_PTR_RST_ (0x80000000) /* WFrame Filter Pointer Rst */
  #define WUCSR_GUE_         (0x00000200) /* Global Unicast Enable */
  #define WUCSR_WUFR_        (0x00000040) /* Wakeup Frame Received */
  #define WUCSR_MPR_         (0x00000020) /* Magic Packet Received */
  #define WUCSR_WAKE_EN_     (0x00000004) /* Wakeup Frame Enable */
  #define WUCSR_MPEN_        (0x00000002) /* Magic Packet Enable */

/* Checksum Offload Engine Control Register */
#define COE_CR (0x130)
  #define Tx_COE_EN_   (0x00010000) /* TX Csum Offload Enable */
  #define Rx_COE_MODE_ (0x00000002) /* RX Csum Offload Mode */
  #define Rx_COE_EN_   (0x00000001) /* RX Csum Offload Enable */

/* Vendor-specific PHY Definitions (via MII access) */
/* EDPD NLP / crossover time configuration (LAN9500A only) */
#define PHY_EDPD_CONFIG (16)
  #define PHY_EDPD_CONFIG_TX_NLP_EN_     (0x8000)
  #define PHY_EDPD_CONFIG_TX_NLP_1000_   (0x0000)
  #define PHY_EDPD_CONFIG_TX_NLP_768_    (0x2000)
  #define PHY_EDPD_CONFIG_TX_NLP_512_    (0x4000)
  #define PHY_EDPD_CONFIG_TX_NLP_256_    (0x6000)
  #define PHY_EDPD_CONFIG_RX_1_NLP_      (0x1000)
  #define PHY_EDPD_CONFIG_RX_NLP_64_     (0x0000)
  #define PHY_EDPD_CONFIG_RX_NLP_256_    (0x0400)
  #define PHY_EDPD_CONFIG_RX_NLP_512_    (0x0800)
  #define PHY_EDPD_CONFIG_RX_NLP_1000_   (0x0C00)
  #define PHY_EDPD_CONFIG_EXT_CROSSOVER_ (0x0001)
  #define PHY_EDPD_CONFIG_DEFAULT (PHY_EDPD_CONFIG_TX_NLP_EN_ |  \
                                  PHY_EDPD_CONFIG_TX_NLP_768_ | \
                                  PHY_EDPD_CONFIG_RX_1_NLP_)

/* Mode Control/Status Register */
#define PHY_MODE_CTRL_STS (17)
  #define MODE_CTRL_STS_EDPWRDOWN_ (0x2000)
  #define MODE_CTRL_STS_ENERGYON_  (0x0002)

/* Control/Status Indication Register */
#define SPECIAL_CTRL_STS (27)
  #define SPECIAL_CTRL_STS_OVRRD_AMDIX_  (0x8000)
  #define SPECIAL_CTRL_STS_AMDIX_ENABLE_ (0x4000)
  #define SPECIAL_CTRL_STS_AMDIX_STATE_  (0x2000)

/* Interrupt Source Register */
#define PHY_INT_SRC (29)
  #define PHY_INT_SRC_ENERGY_ON_    (0x0080)
  #define PHY_INT_SRC_ANEG_COMP_    (0x0040)
  #define PHY_INT_SRC_REMOTE_FAULT_ (0x0020)
  #define PHY_INT_SRC_LINK_DOWN_    (0x0010)

/* Interrupt Mask Register */
#define PHY_INT_MASK (30)
  #define PHY_INT_MASK_ENERGY_ON_    (0x0080)
  #define PHY_INT_MASK_ANEG_COMP_    (0x0040)
  #define PHY_INT_MASK_REMOTE_FAULT_ (0x0020)
  #define PHY_INT_MASK_LINK_DOWN_    (0x0010)
  #define PHY_INT_MASK_DEFAULT_ (PHY_INT_MASK_ANEG_COMP_ | \
                                PHY_INT_MASK_LINK_DOWN_)
/* PHY Special Control/Status Register */
#define PHY_SPECIAL (31)
  #define PHY_SPECIAL_SPD_         (0x001C)
  #define PHY_SPECIAL_SPD_10HALF_  (0x0004)
  #define PHY_SPECIAL_SPD_10FULL_  (0x0014)
  #define PHY_SPECIAL_SPD_100HALF_ (0x0008)
  #define PHY_SPECIAL_SPD_100FULL_ (0x0018)

/* USB Vendor Requests */
#define USB_VENDOR_REQUEST_WRITE_REGISTER 0xA0
#define USB_VENDOR_REQUEST_READ_REGISTER  0xA1
#define USB_VENDOR_REQUEST_GET_STATS      0xA2

#define BIT(x)   (1 << x)

/* Interrupt Endpoint status word bitfields */
#define INT_ENP_MAC_RTO_ ((UINT32)BIT(18)) /* MAC Reset Time Out */
#define INT_ENP_TX_STOP_ ((UINT32)BIT(17)) /* TX Stopped */
#define INT_ENP_RX_STOP_ ((UINT32)BIT(16)) /* RX Stopped */
#define INT_ENP_PHY_INT_ ((UINT32)BIT(15)) /* PHY Interrupt */
#define INT_ENP_TXE_     ((UINT32)BIT(14)) /* TX Error */
#define INT_ENP_TDFU_    ((UINT32)BIT(13)) /* TX FIFO Underrun */
#define INT_ENP_TDFO_    ((UINT32)BIT(12)) /* TX FIFO Overrun */
#define INT_ENP_RXDF_    ((UINT32)BIT(11)) /* RX Dropped Frame */



/* Generic MII registers. */
#define MII_BMCR        0x00 /* Basic mode control register */
#define MII_BMSR        0x01 /* Basic mode status register  */
#define MII_PHYSID1     0x02 /* PHYS ID 1                   */
#define MII_PHYSID2     0x03 /* PHYS ID 2                   */
#define MII_ADVERTISE   0x04 /* Advertisement control reg   */
#define MII_LPA         0x05 /* Link partner ability reg    */
#define MII_EXPANSION   0x06 /* Expansion register          */
#define MII_CTRL1000    0x09 /* 1000BASE-T control          */
#define MII_STAT1000    0x0a /* 1000BASE-T status           */
#define MII_MMD_CTRL    0x0d /* MMD Access Control Register */
#define MII_MMD_DATA    0x0e /* MMD Access Data Register */
#define MII_ESTATUS     0x0f /* Extended Status             */
#define MII_DCOUNTER    0x12 /* Disconnect counter          */
#define MII_FCSCOUNTER  0x13  /* False carrier counter       */
#define MII_NWAYTEST    0x14 /* N-way auto-neg test reg     */
#define MII_RERRCOUNTER 0x15 /* Receive error counter       */
#define MII_SREVISION   0x16 /* Silicon revision            */
#define MII_RESV1       0x17 /* Reserved...                 */
#define MII_LBRERROR    0x18 /* Lpback, rx, bypass error    */
#define MII_PHYADDR     0x19 /* PHY address                 */
#define MII_RESV2       0x1a /* Reserved...                 */
#define MII_TPISTATUS   0x1b /* TPI status for 10mbps       */
#define MII_NCONFIG     0x1c /* Network interface config    */

/* Basic mode control register. */
#define BMCR_RESV      0x003f /* Unused...                   */
#define BMCR_SPEED1000 0x0040 /* MSB of Speed (1000)         */
#define BMCR_CTST      0x0080 /* Collision test              */
#define BMCR_FULLDPLX  0x0100 /* Full duplex                 */
#define BMCR_ANRESTART 0x0200 /* Auto negotiation restart    */
#define BMCR_ISOLATE   0x0400 /* Isolate data paths from MII */
#define BMCR_PDOWN     0x0800 /* Enable low power state      */
#define BMCR_ANENABLE  0x1000 /* Enable auto negotiation     */
#define BMCR_SPEED100  0x2000 /* Select 100Mbps              */
#define BMCR_LOOPBACK  0x4000 /* TXD loopback bits           */
#define BMCR_RESET     0x8000 /* Reset to default state      */
#define BMCR_SPEED10   0x0000 /* Select 10Mbps               */

/* Basic mode status register. */
#define BMSR_ERCAP        0x0001 /* Ext-reg capability          */
#define BMSR_JCD          0x0002 /* Jabber detected             */
#define BMSR_LSTATUS      0x0004 /* Link status                 */
#define BMSR_ANEGCAPABLE  0x0008 /* Able to do auto-negotiation */
#define BMSR_RFAULT       0x0010 /* Remote fault detected       */
#define BMSR_ANEGCOMPLETE 0x0020 /* Auto-negotiation complete   */
#define BMSR_RESV         0x00c0 /* Unused...                   */
#define BMSR_ESTATEN      0x0100 /* Extended Status in R15      */
#define BMSR_100HALF2     0x0200 /* Can do 100BASE-T2 HDX       */
#define BMSR_100FULL2     0x0400 /* Can do 100BASE-T2 FDX       */
#define BMSR_10HALF       0x0800 /* Can do 10mbps, half-duplex  */
#define BMSR_10FULL       0x1000 /* Can do 10mbps, full-duplex  */
#define BMSR_100HALF      0x2000 /* Can do 100mbps, half-duplex */
#define BMSR_100FULL      0x4000 /* Can do 100mbps, full-duplex */
#define BMSR_100BASE4     0x8000 /* Can do 100mbps, 4k packets  */

/* Advertisement control register. */
#define ADVERTISE_SLCT          0x001f /* Selector bits               */
#define ADVERTISE_CSMA          0x0001 /* Only selector supported     */
#define ADVERTISE_10HALF        0x0020 /* Try for 10mbps half-duplex  */
#define ADVERTISE_1000XFULL     0x0020 /* Try for 1000BASE-X full-duplex */
#define ADVERTISE_10FULL        0x0040 /* Try for 10mbps full-duplex  */
#define ADVERTISE_1000XHALF     0x0040 /* Try for 1000BASE-X half-duplex */
#define ADVERTISE_100HALF       0x0080 /* Try for 100mbps half-duplex */
#define ADVERTISE_1000XPAUSE    0x0080 /* Try for 1000BASE-X pause    */
#define ADVERTISE_100FULL       0x0100 /* Try for 100mbps full-duplex */
#define ADVERTISE_1000XPSE_ASYM 0x0100 /* Try for 1000BASE-X asym pause */
#define ADVERTISE_100BASE4      0x0200 /* Try for 100mbps 4k packets  */
#define ADVERTISE_PAUSE_CAP     0x0400 /* Try for pause               */
#define ADVERTISE_PAUSE_ASYM    0x0800 /* Try for asymetric pause     */
#define ADVERTISE_RESV          0x1000 /* Unused...                   */
#define ADVERTISE_RFAULT        0x2000 /* Say we can detect faults    */
#define ADVERTISE_LPACK         0x4000 /* Ack link partners response  */
#define ADVERTISE_NPAGE         0x8000 /* Next page bit               */

#define ADVERTISE_FULL (ADVERTISE_100FULL | ADVERTISE_10FULL | \
                        ADVERTISE_CSMA)
#define ADVERTISE_ALL (ADVERTISE_10HALF | ADVERTISE_10FULL | \
                       ADVERTISE_100HALF | ADVERTISE_100FULL)


EFI_STATUS
Smsc95xxPhyRead (
  IN     NIC_DEVICE *NicDevice,
  IN     UINT32     RegisterAddress,
  IN OUT UINT32     *PhyData
  );

EFI_STATUS
Smsc95xxPhyWrite (
  IN NIC_DEVICE *NicDevice,
  IN UINT32     RegisterAddress,
  IN UINT32     PhyData
  );

BOOLEAN
Smsc95xxGetLinkStatus (
  IN NIC_DEVICE *NicDevice
);

VOID
Smsc95xxDumpRegs (
  IN       NIC_DEVICE *NicDevice
  );