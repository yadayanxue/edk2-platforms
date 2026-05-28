/** @file
  Implement the interface to the Smsc95xx Ethernet controller.

  This module implements the interface to the MicroChip Smsc95xx
  USB to Ethernet MAC with integrated 10/100 PHY.  Note that this implementation
  only supports the integrated PHY since no other test cases were available.

  Copyright (c) 2011, Intel Corporation. All rights reserved.
  Copyright (c) 2020, ARM Limited. All rights reserved.

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "Smsc95xx.h"

EFI_STATUS
Smsc95xxReadReg (
  IN       NIC_DEVICE *NicDevice,
  IN       UINT16     Index,
  IN  OUT  UINT32     *Data
  )
{
  EFI_STATUS         Status;
  USB_DEVICE_REQUEST SetupMsg;

  SetupMsg.RequestType = USB_REQ_TYPE_VENDOR
                       | USB_TARGET_DEVICE
                       | USB_ENDPOINT_DIR_IN;

  SetupMsg.Request = USB_VENDOR_REQUEST_READ_REGISTER;
  SetupMsg.Value  = 0;
  SetupMsg.Index  = Index;
  SetupMsg.Length = 4;

  Status = Smsc95xxUsbCommand (NicDevice,
                               &SetupMsg,
                               Data);

  return Status;
}

EFI_STATUS
Smsc95xxWriteReg (
  IN       NIC_DEVICE *NicDevice,
  IN       UINT16     Index,
  IN       UINT32     Data
  )
{
  EFI_STATUS         Status;
  USB_DEVICE_REQUEST SetupMsg;

  SetupMsg.RequestType = USB_REQ_TYPE_VENDOR
                       | USB_TARGET_DEVICE;

  SetupMsg.Request = USB_VENDOR_REQUEST_WRITE_REGISTER;
  SetupMsg.Value  = 0;
  SetupMsg.Index  = Index;
  SetupMsg.Length = 4;

  Status = Smsc95xxUsbCommand (NicDevice,
                               &SetupMsg,
                               &Data);

  return Status;
}

EFI_STATUS
Smsc95xxSetCsums (
  IN       NIC_DEVICE *NicDevice,
  IN       BOOLEAN    UseTxCsum,
  IN       BOOLEAN    UseRxCsum
  )
{
  EFI_STATUS         Status;
  UINT32             Data;

  Status = Smsc95xxReadReg(NicDevice, COE_CR, &Data);
  if (EFI_ERROR(Status)) {
    return Status;
  }

  if (UseTxCsum) {
    Data |= Tx_COE_EN_;
  } else {
    Data &= ~Tx_COE_EN_;
  }
  if (UseRxCsum) {
    Data |= Rx_COE_EN_;
  } else {
    Data &= ~Rx_COE_EN_;
  }

  Status = Smsc95xxWriteReg(NicDevice, COE_CR, Data);
  if (EFI_ERROR(Status)) {
    return Status;
  }

  DEBUG ((DEBUG_INFO, "COE_CR: 0x%08x\n", Data));
  return EFI_TIMEOUT;
}

EFI_STATUS
Smsc95xxPhyWaitNotBusy (
  IN       NIC_DEVICE *NicDevice
  )
{
  UINT32 Data;
  UINT64 Start, Stop;

  gBS->GetNextMonotonicCount(&Start);
  Stop = Start + 1 * 1000 * 10000;

  do {
    Smsc95xxReadReg(NicDevice, MII_ADDR, &Data);
    if (!(Data & MII_BUSY_)) {
      return EFI_SUCCESS;
    }
    gBS->GetNextMonotonicCount(&Start);
  } while (Start < Stop);

  return EFI_TIMEOUT;
}

VOID
Smsc95xxDumpRegs (
  IN       NIC_DEVICE *NicDevice
  )
{
  UINT32 Data;
  INT32  Index, Col, Row;

  // for (Index = 0, Row = 0; Index < 0x1000;) {
  for (Index = ID_REV, Row = 0; Index <= COE_CR;) {
    DEBUG ((DEBUG_INFO, "0x%04x:         ", Row));
    for (Col = 0; Col < 4 && Index <= COE_CR; Col++, Index += 4) {
      Smsc95xxReadReg(NicDevice, Index, &Data);
      DEBUG ((DEBUG_INFO, " %08x", Data));
    }
    DEBUG ((DEBUG_INFO, "\n"));
    Row += 4 * 4;
  }
}

/**
  Compute the CRC

  @param [in] MacAddress      Address of a six byte buffer to containing the MAC address.

  @returns The CRC-32 value associated with this MAC address

**/
UINT32
Smsc95xxCrc (
  IN UINT8 *MacAddress
  )
{
  UINT32 BitNumber;
  INT32  Carry;
  INT32  Crc;
  UINT32 Data;
  UINT8  *End;

  //
  //  Walk the MAC address
  //
  Crc = -1;
  End = &MacAddress[PXE_HWADDR_LEN_ETHER];
  while (End > MacAddress) {
    Data = *MacAddress++;


    //
    //  CRC32: x32 + x26 + x23 + x22 + x16 + x12 + x11 + x10 + x8 + x7 + x5 + x4 + x2 + x + 1
    //
    //          1 0000 0100 1100 0001 0001 1101 1011 0111
    //
    for (BitNumber = 0; 8 > BitNumber; BitNumber++) {
      Carry = ((Crc >> 31) & 1) ^ (Data & 1);
      Crc <<= 1;
      if (Carry != 0) {
        Crc ^= 0x04c11db7;
      }
      Data >>= 1;
    }
  }

  //
  //  Return the CRC value
  //
  return (UINT32) Crc & 0x3f;
}

EFI_STATUS
Smsc95xxMacAddressInit (
  OUT UINT8      *MacAddress
  )
{
  EFI_STATUS  Status;
  CONST UINT8 *List;
  INT32       ListSize;
  VOID        *Dtb;
  INTN        Node;

  Status = EfiGetSystemConfigurationTable (&gFdtTableGuid, &Dtb);
  if (EFI_SUCCESS == Status) {
    Node = FdtPathOffset(Dtb, "ethernet");
    if (Node >= 1) {
      List = FdtGetProp(Dtb, Node, "mac-address", &ListSize);
      if (List == NULL) {
        Status = EFI_NOT_FOUND;
      }
      else {
        CopyMem (MacAddress, List, PXE_HWADDR_LEN_ETHER);
      }
    } else {
      Status = EFI_NOT_FOUND;
    }
  }

  return Status;
}

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
  IN  NIC_DEVICE *NicDevice,
  OUT UINT8      *MacAddress
  )
{
  EFI_STATUS Status;
  UINT32     Data[2];

  Status = Smsc95xxReadReg (NicDevice, ADDRL, &Data[0]);
  if (EFI_ERROR (Status))
  {
    return Status;
  }
  Status = Smsc95xxReadReg (NicDevice, ADDRH, &Data[1]);
  CopyMem(MacAddress, Data, PXE_HWADDR_LEN_ETHER);
  return Status;
}

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
  )
{
  EFI_STATUS Status;
  UINT32     addr_lo, addr_hi;

  addr_lo = MacAddress[0] | MacAddress[1] << 8 | MacAddress[2] << 16 | MacAddress[3] << 24;
  addr_hi = MacAddress[4] | MacAddress[5] << 8;
  Status = Smsc95xxWriteReg (NicDevice, ADDRL, addr_lo);
  if (EFI_ERROR (Status))
  {
    return Status;
  }
  Status = Smsc95xxWriteReg (NicDevice, ADDRH, addr_hi);
  return Status;
}

/**
  Clear the multicast hash table

  @param [in] NicDevice       Pointer to the NIC_DEVICE structure

**/
VOID
Smsc95xxMulticastClear (
  IN NIC_DEVICE *NicDevice
  )
{
  int Index = 0;
  //
  // Clear the multicast hash table
  //
  for (Index = 0 ; Index < 8 ; Index ++)
    NicDevice->MulticastHash[Index] = 0;
}

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
  )
{
  UINT32 Crc;

  //
  // Compute the CRC on the destination address
  //
  Crc = Smsc95xxCrc (MacAddress) >> 26;

  //
  //  Set the bit corresponding to the destination address
  //
  NicDevice->MulticastHash [Crc >> 3] |= (1 << (Crc & 7));

}

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
  )
{
  EFI_STATUS Status;
  UINT32     PhyData;

  PhyData = BMCR_ANENABLE | BMCR_ANRESTART;
  if (NicDevice->LinkSpeed100Mbps) {
    PhyData |= BMCR_SPEED100;
  }
  if (NicDevice->FullDuplex) {
    PhyData |= BMCR_FULLDPLX;
  }
  Status = Smsc95xxPhyWrite (NicDevice, MII_BMCR, PhyData);

  UINT32 Index = 0;
  do {
    Index++;
    DEBUG ((DEBUG_ERROR, "Waiting for Ethernet connection... : %d\n", Index));
    gBS->Stall(500 * 1000);
    Smsc95xxPhyRead (NicDevice, MII_BMSR, &PhyData);
  } while (!(PhyData & BMSR_LSTATUS) && (Index < 10));
  if (Index >= 10) {
    DEBUG ((DEBUG_ERROR, "[%a:%d -> %a] %r, timeout waiting for nego link start\n", __FILE_NAME__, DEBUG_LINE_NUMBER, __func__, Status));
  }

  Smsc95xxDumpRegs(NicDevice);

  DEBUG ((DEBUG_INFO, "  [%a:%d -> %a] %r\n", __FILE_NAME__, DEBUG_LINE_NUMBER, __func__, Status));
  return Status;
}



/**
  Complete the negotiation of the PHY link

  This routine calls ::Smsc95xxPhyRead to determine if the
  link negotiation is complete.

  @param [in] NicDevice       Pointer to the NIC_DEVICE structure
  @param [in, out] PollCount  Address of number of times this routine was polled
  @param [out] Complete       Address of boolean to receive complate status.
  @param [out] LinkUp         Address of boolean to receive link status, TRUE=up.
  @param [out] HiSpeed        Address of boolean to receive link speed, TRUE=100Mbps.
  @param [out] FullDuplex     Address of boolean to receive link duplex, TRUE=full.

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
  )
{
  EFI_STATUS  Status;
  UINT32      PhyData;
  UINT16      Mask;

  //
  //  Determine if the link is up.
  //
  *Complete = FALSE;

  //
  //  Get the link status
  //
  Status = Smsc95xxPhyRead (NicDevice, MII_BMSR, &PhyData);
  // DEBUG ((DEBUG_INFO, "  [%a:%d -> %a] %r MII_BMSR: 0x%08x\n", __FILE_NAME__, DEBUG_LINE_NUMBER, __func__, Status, PhyData));
  if (EFI_ERROR (Status)) {
    return Status;
  }

  *LinkUp = ((PhyData & BMSR_LSTATUS) != 0);
  if (*LinkUp) {
    *Complete = ((PhyData & BMSR_AUTONEG_CMPLT) != 0);
    if (*Complete) {
      Status = Smsc95xxPhyRead (NicDevice, PHY_ANLPAR, &PhyData);
      // DEBUG ((DEBUG_INFO, "  [%a:%d -> %a] %r PHY_ANLPAR: 0x%08x\n", __FILE_NAME__, DEBUG_LINE_NUMBER, __func__, Status, PhyData));
      if (EFI_ERROR (Status)) {
        return Status;
      }
      //
      //  Autonegotiation is complete
      //  Determine the link speed.
      //
      *HiSpeed = ((PhyData & (AN_TX_FDX | AN_TX_HDX))!= 0);
      //
      //  Determine the link duplex.
      //
      Mask = (*HiSpeed) ? AN_TX_FDX : AN_10_FDX;
      *FullDuplex = (BOOLEAN)((PhyData & Mask) != 0);
    }
  }

  // DEBUG ((DEBUG_INFO, "  [%a:%d -> %a] %r PollCount: %d, Complete: %d, LinkUp: %d, HiSpeed: %d, FullDuplex: %d\n", __FILE_NAME__, DEBUG_LINE_NUMBER, __func__, Status, *PollCount, *Complete, *LinkUp, *HiSpeed, *FullDuplex));
  return Status;
}

/**
  Read a register from the PHY

  This routine calls ::Smsc95xxUsbCommand to read a PHY register.

  @param [in] NicDevice       Pointer to the NIC_DEVICE structure
  @param [in] RegisterAddress  Number of the register to read.
  @param [in, out] PhyData    Address of a buffer to receive the PHY register value

  @retval EFI_SUCCESS          The PHY data is available.
  @retval other                The PHY data is not valid.

**/
EFI_STATUS
Smsc95xxPhyRead (
  IN     NIC_DEVICE *NicDevice,
  IN     UINT32     RegisterAddress,
  IN OUT UINT32     *PhyData
  )
{
  EFI_STATUS         Status;

  /* confirm MII not busy */
  Status = Smsc95xxPhyWaitNotBusy(NicDevice);
  if (EFI_ERROR (Status)) {
    goto err;
  }

  /* set the address, index & direction (read from PHY) */
  Status = Smsc95xxWriteReg(NicDevice, MII_ADDR, (NicDevice->PhyId << 11) | (RegisterAddress << 6) | MII_READ_);
  if (EFI_ERROR (Status)) {
    goto err;
  }

  Status = Smsc95xxPhyWaitNotBusy(NicDevice);
  if (EFI_ERROR (Status)) {
    goto err;
  }

  Status = Smsc95xxReadReg(NicDevice, MII_DATA, PhyData);

err:
  // DEBUG ((DEBUG_INFO, "  [%a:%d -> %a] %r\n", __FILE_NAME__, DEBUG_LINE_NUMBER, __func__, Status));
  return Status;
}

/**
  Write to a PHY register

  This routine calls ::Smsc95xxUsbCommand to write a PHY register.

  @param [in] NicDevice       Pointer to the NIC_DEVICE structure
  @param [in] RegisterAddress  Number of the register to read.
  @param [in] PhyData          Address of a buffer to receive the PHY register value

  @retval EFI_SUCCESS          The PHY data was written.
  @retval other                Failed to wwrite the PHY register.

**/
EFI_STATUS
Smsc95xxPhyWrite (
  IN NIC_DEVICE *NicDevice,
  IN UINT32     RegisterAddress,
  IN UINT32     PhyData
  )
{
  EFI_STATUS    Status;

  /* confirm MII not busy */
  Status = Smsc95xxPhyWaitNotBusy(NicDevice);
  if (EFI_ERROR (Status)) {
    goto err;
  }

  Status = Smsc95xxWriteReg(NicDevice, MII_DATA, PhyData);
  if (EFI_ERROR (Status)) {
    goto err;
  }

  /* set the address, index & direction (write to PHY) */
  Status = Smsc95xxWriteReg(NicDevice, MII_ADDR, (NicDevice->PhyId << 11) | (RegisterAddress << 6) | MII_WRITE_);

  Status = Smsc95xxPhyWaitNotBusy(NicDevice);

err:
  // DEBUG ((DEBUG_INFO, "  [%a:%d -> %a] %r\n", __FILE_NAME__, DEBUG_LINE_NUMBER, __func__, Status));
  return Status;
}


/**
  Reset the SMSC95xx

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
  )
{
  EFI_STATUS          Status;
  UINT32              Data;
  INT32               Index;

  DEBUG ((DEBUG_ERROR, "[%a:%d -> %a] entering smsc95xx_reset\n", __FILE_NAME__, DEBUG_LINE_NUMBER, __func__));

  // 1. 复位
  Status = Smsc95xxWriteReg(NicDevice, HW_CFG, HW_CFG_LRST_);
  if (EFI_ERROR(Status)) {
    goto err;
  }
  Index = 0;
  do {
    gBS->Stall(10 * 1000);
    Status = Smsc95xxReadReg(NicDevice, HW_CFG, &Data);
    if (EFI_ERROR(Status)) {
      goto err;
    }
    Index++;
  } while ((Data & HW_CFG_LRST_) && (Index < 100));
  if (Index >= 100) {
    Status = EFI_TIMEOUT;
    DEBUG ((DEBUG_ERROR, "[%a:%d -> %a] %r, timeout waiting for completion of Lite Reset\n", __FILE_NAME__, DEBUG_LINE_NUMBER, __func__, Status));
    goto err;
  }

  // 2. power rst
  Data = PM_CTL_PHY_RST_;
  Status = Smsc95xxWriteReg(NicDevice, PM_CTRL, PM_CTL_PHY_RST_);
  if (EFI_ERROR(Status)) {
    goto err;
  }
  Index = 0;
  do {
    gBS->Stall(10 * 1000);
    Status = Smsc95xxReadReg(NicDevice, PM_CTRL, &Data);
    if (EFI_ERROR(Status)) {
      goto err;
    }
    Index++;
  } while ((Data & HW_CFG_LRST_) && (Index < 100));
  if (Index >= 100) {
    Status = EFI_TIMEOUT;
    DEBUG ((DEBUG_ERROR, "[%a:%d -> %a] %r, timeout waiting for PHY Reset\n", __FILE_NAME__, DEBUG_LINE_NUMBER, __func__, Status));
    goto err;
  }

  // 3. 配置 HWADDR mac-address
  Status = Smsc95xxMacAddressSet(NicDevice, &NicDevice->SimpleNetworkData.CurrentAddress.Addr[0]);
  if (EFI_ERROR (Status)) {
    DEBUG((DEBUG_ERROR, "Error: SMSC95xx: Failed to set MAC address\n"));
    goto err;
  }

  // 4。这个配置会导致程序卡住，拔掉网线恢复
  // Status = Smsc95xxReadReg(NicDevice, HW_CFG, &Data);
  // if (EFI_ERROR(Status)) {
  //   goto err;
  // }
  // DEBUG ((DEBUG_INFO, "Read Value from HW_CFG: 0x%08x\n", Data));

  // Data |= HW_CFG_BIR_;
  // Status = Smsc95xxWriteReg(NicDevice, HW_CFG, Data);
  // if (EFI_ERROR(Status)) {
  //   goto err;
  // }
  // Status = Smsc95xxReadReg(NicDevice, HW_CFG, &Data);
  // if (EFI_ERROR(Status)) {
  //   goto err;
  // }
  // DEBUG ((DEBUG_INFO, "Read Value from HW_CFG after writing HW_CFG_BIR_: 0x%08x\n", Data));

  // 5. brust_cap
  Data = 0;

#ifdef TURBO_MODE
  // Data = DEFAULT_HS_BURST_CAP_SIZE / HS_USB_PKT_SIZE;
  // Data = DEFAULT_FS_BURST_CAP_SIZE / FS_USB_PKT_SIZE;
#endif

  Status = Smsc95xxWriteReg(NicDevice, BURST_CAP, Data);
  if (EFI_ERROR(Status)) {
    goto err;
  }
  Status = Smsc95xxReadReg (NicDevice, BURST_CAP, &Data);
  if (EFI_ERROR(Status)) {
    goto err;
  }
  DEBUG ((DEBUG_INFO, "Read Value from BURST_CAP after writing: 0x%08x\n", Data));

  // 6. 
  Status = Smsc95xxWriteReg(NicDevice, BULK_IN_DLY, DEFAULT_BULK_IN_DELAY);
  if (EFI_ERROR(Status)) {
    goto err;
  }
  Status = Smsc95xxReadReg (NicDevice, BULK_IN_DLY, &Data);
  if (EFI_ERROR(Status)) {
    goto err;
  }
  DEBUG ((DEBUG_INFO, "Read Value from BULK_IN_DLY after writing: 0x%08x\n", Data));

  // 7. 
  Status = Smsc95xxReadReg(NicDevice, HW_CFG, &Data);
  if (EFI_ERROR(Status)) {
    goto err;
  }
  DEBUG ((DEBUG_INFO, "Read Value from HW_CFG: 0x%08x\n", Data));

#ifdef TURBO_MODE
  Data |= (HW_CFG_MEF_ | HW_CFG_BCE_);
#endif

  Data &= ~HW_CFG_RXDOFF_;

  // 接收接口Buffer地址结尾是 E，拷贝14字节后是 C，刚好4字节对齐，所以无需偏移处理
#define NET_IP_ALIGN 0
  Data |= NET_IP_ALIGN << 9;

  Status = Smsc95xxWriteReg(NicDevice, HW_CFG, Data);
  if (EFI_ERROR(Status)) {
    goto err;
  }
  Status = Smsc95xxReadReg (NicDevice, HW_CFG, &Data);
  if (EFI_ERROR(Status)) {
    goto err;
  }
  DEBUG ((DEBUG_INFO, "Read Value from HW_CFG after writing: 0x%08x\n", Data));

  // 8. 清中断标志
  Status = Smsc95xxWriteReg(NicDevice, INT_STS, INT_STS_CLEAR_ALL_);
  if (EFI_ERROR(Status)) {
    goto err;
  }

  // 9. 
  Status = Smsc95xxReadReg(NicDevice, ID_REV, &Data);
  if (EFI_ERROR(Status)) {
    goto err;
  }
  DEBUG ((DEBUG_INFO, "ID_REV = 0x%08x\n", Data));

  // 10. 开启网口提示灯
  // Status = Smsc95xxReadReg(NicDevice, LED_GPIO_CFG, &Data);
  // if (EFI_ERROR(Status)) {
  //   goto err;
  // }
  /* Configure GPIO pins as LED outputs */
  Data = LED_GPIO_CFG_SPD_LED | LED_GPIO_CFG_LNK_LED | LED_GPIO_CFG_FDX_LED;
  Status = Smsc95xxWriteReg(NicDevice, LED_GPIO_CFG, Data);
  if (EFI_ERROR(Status)) {
    goto err;
  }

  // 11. 
  /* Init Tx */
  Status = Smsc95xxWriteReg(NicDevice, FLOW, 0);
  if (EFI_ERROR(Status)) {
    goto err;
  }

  Status   = Smsc95xxWriteReg(NicDevice, AFC_CFG, AFC_CFG_DEFAULT);
  if (EFI_ERROR(Status)) {
    goto err;
  }

  /* Don't need mac_cr_lock during initialisation */
  Status = Smsc95xxReadReg(NicDevice, MAC_CR, &NicDevice->MacCR);
  if (EFI_ERROR(Status)) {
    goto err;
  }

  DEBUG ((DEBUG_INFO, "MAC_CR = 0x%08x\n", NicDevice->MacCR));

  /* Init Rx. Set Vlan */
  Status = Smsc95xxWriteReg(NicDevice, VLAN1, ETH_P_8021Q);
  if (EFI_ERROR(Status)) {
    goto err;
  }

  /* Enable or disable checksum offload engines */
  Status = Smsc95xxSetCsums(NicDevice, FALSE, FALSE);
  if (Status < 0) {
    DEBUG ((EFI_D_ERROR, "Failed to set csum offload: %r\n", Status));
    return Status;
  }

  /* No multicast */
  NicDevice->MacCR &= ~(MAC_CR_PRMS_ | MAC_CR_MCPAS_ | MAC_CR_HPFILT_);
  NicDevice->MacCR |= MAC_CR_FDPX_;

  // 复位phy
  Status = Smsc95xxPhyWrite (NicDevice, MII_BMCR, BMCR_RESET);
  if (EFI_ERROR (Status))
  {
    goto err;
  }
  Index = 0;
  do {
    gBS->Stall(10 * 1000);
    Status = Smsc95xxPhyRead(NicDevice, MII_BMCR, &Data);
    if (EFI_ERROR(Status)) {
      goto err;
    }
    Index++;
  } while ((Data & BMCR_RESET) && (Index < 100));
  if (Index >= 100) {
    Status = EFI_TIMEOUT;
    DEBUG ((DEBUG_ERROR, "[%a:%d -> %a] %r, timeout waiting for PHY BMCR Reset\n", __FILE_NAME__, DEBUG_LINE_NUMBER, __func__, Status));
    goto err;
  }
  Status = Smsc95xxPhyWrite (NicDevice, MII_ADVERTISE, ADVERTISE_ALL | ADVERTISE_CSMA | ADVERTISE_PAUSE_CAP | ADVERTISE_PAUSE_ASYM);
  if (EFI_ERROR (Status))
  {
    goto err;
  }
  /* read to clear */
  Status = Smsc95xxPhyRead (NicDevice, PHY_INT_SRC, &Data);
  if (EFI_ERROR (Status))
  {
    goto err;
  }
  Status = Smsc95xxPhyWrite (NicDevice, PHY_INT_MASK, PHY_INT_MASK_DEFAULT_);
  if (EFI_ERROR (Status))
  {
    goto err;
  }
  // 
  Status = Smsc95xxReadReg(NicDevice, INT_EP_CTL, &Data);
  if (EFI_ERROR(Status)) {
    goto err;
  }
  /* enable PHY interrupts */
  Data |= INT_EP_CTL_PHY_INT_;
  Status = Smsc95xxWriteReg(NicDevice, INT_EP_CTL, Data);
  if (EFI_ERROR(Status)) {
    goto err;
  }

  // 
  // Smsc95xxStartTxPath(NicDevice, MacCr);
  // Smsc95xxStartRxPath(NicDevice, MacCr);
  /* Enable Tx at MAC */
  NicDevice->MacCR |= MAC_CR_TXEN_;
  Status = Smsc95xxWriteReg(NicDevice, MAC_CR, NicDevice->MacCR);
  /* Enable Tx at SCSRs */
  Status = Smsc95xxWriteReg(NicDevice, TX_CFG, TX_CFG_ON_);
  /* Enable Rx at MAC */
  NicDevice->MacCR |= MAC_CR_RXEN_;
  Status = Smsc95xxWriteReg(NicDevice, MAC_CR, NicDevice->MacCR);

  Smsc95xxDumpRegs(NicDevice);

err:
  DEBUG ((DEBUG_INFO, "  [%a:%d -> %a] %r\n", __FILE_NAME__, DEBUG_LINE_NUMBER, __func__, Status));
  return Status;
}

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
  )
{
  EFI_STATUS    Status = EFI_SUCCESS;
  UINT16        RxControl;

  // DEBUG ((DEBUG_INFO, "  [%a:%d -> %a] RxFilter: 0x%x\n", __FILE_NAME__, DEBUG_LINE_NUMBER, __func__, RxFilter));

  //
  // Enable the receiver if something is to be received
  //
  if (RxFilter != 0) {
    //
    //  Enable the receiver
    //
  }
  RxControl = RXC_SO;
  //
  //  Enable multicast if requested
  //
  if ((RxFilter & EFI_SIMPLE_NETWORK_RECEIVE_MULTICAST) != 0) {
    RxControl |= RXC_AM;

    if (EFI_ERROR(Status))
      goto EXIT;
  }

  //
  //  Enable all multicast if requested
  //
  if ((RxFilter & EFI_SIMPLE_NETWORK_RECEIVE_PROMISCUOUS_MULTICAST) != 0) {
    RxControl |= RXC_AMALL;
  }
  //
  //  Enable broadcast if requested
  //
  if ((RxFilter & EFI_SIMPLE_NETWORK_RECEIVE_BROADCAST) != 0) {
    RxControl |= RXC_AB;
  }

  //
  //  Enable promiscuous mode if requested
  //
  if ((RxFilter & EFI_SIMPLE_NETWORK_RECEIVE_PROMISCUOUS) != 0) {
    RxControl |= RXC_PRO;
  }

  // DEBUG ((DEBUG_INFO, "  [%a:%d -> %a] CurRxControl: %d -> %d\n", __FILE_NAME__, DEBUG_LINE_NUMBER, __func__, NicDevice->CurRxControl, RxControl));
  //
  //  Update the receiver control
  //
  if (NicDevice->CurRxControl != RxControl) {
    if (!EFI_ERROR (Status))
      NicDevice->CurRxControl = RxControl;
  }

  //
  // Return the operation status
  //
EXIT:
  // DEBUG ((DEBUG_INFO, "  [%a:%d -> %a] %r\n", __FILE_NAME__, DEBUG_LINE_NUMBER, __func__, Status));
  return Status;
}



EFI_STATUS
Smsc95xxReloadSrom  (
  IN NIC_DEVICE *NicDevice
  )
{
  EFI_STATUS Status;

  Status = EFI_UNSUPPORTED;
  return Status;

}

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
  )
{
  EFI_STATUS Status;

  Status = EFI_UNSUPPORTED;
  return Status;
}

EFI_STATUS
Smsc95xxEnableSromWrite  (
  IN NIC_DEVICE * NicDevice
  )
{
  EFI_STATUS Status;

  Status = EFI_UNSUPPORTED;
  return Status;
}


EFI_STATUS
Smsc95xxDisableSromWrite  (
  IN NIC_DEVICE *NicDevice
  )
{
  EFI_STATUS Status;

  Status = EFI_UNSUPPORTED;
  return Status;
}

/**
  Write an SROM location

  This routine calls ::Smsc95xxUsbCommand to write data from the
  SROM.

  @param [in] NicDevice       Pointer to the NIC_DEVICE structure
  @param [in] Address          SROM address
  @param [out] Data           Buffer of data to write

  @retval EFI_SUCCESS          The write was successful
  @retval other                The write failed

**/
EFI_STATUS
Smsc95xxSromWrite (
  IN NIC_DEVICE *NicDevice,
  IN UINT32     Address,
  IN UINT16     *Data
  )
{
  EFI_STATUS Status;

  Status = EFI_UNSUPPORTED;
  return Status;
}

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
  IN     NIC_DEVICE         *NicDevice,
  IN     USB_DEVICE_REQUEST *Request,
  IN OUT VOID               *Buffer
  )
{
  EFI_USB_DATA_DIRECTION Direction;
  EFI_USB_IO_PROTOCOL    *UsbIo;
  EFI_STATUS             Status = EFI_TIMEOUT;
  UINT32                 CmdStatus = EFI_USB_NOERROR;
  int                    i;
  //
  // Determine the transfer direction
  //
  Direction = EfiUsbNoData;
  if (Request->Length != 0) {
    Direction = ((Request->RequestType & USB_ENDPOINT_DIR_IN) != 0)
                        ? EfiUsbDataIn : EfiUsbDataOut;
  }

  //
  // Issue the command
  //
  UsbIo = NicDevice->UsbIo;

  for (i = 0 ; i < 3 && EFI_TIMEOUT == Status; i++) {
    Status = UsbIo->UsbControlTransfer (UsbIo,
                                        Request,
                                        Direction,
                                        USB_BUS_TIMEOUT,
                                        Buffer,
                                        Request->Length,
                                        &CmdStatus);
  }
  //
  // Determine the operation status
  //
  if (EFI_ERROR(Status) || EFI_ERROR(CmdStatus))
    Status = EFI_DEVICE_ERROR;
  //
  // Return the operation status
  //
  return Status;
}

BOOLEAN
Smsc95xxGetLinkStatus (
  IN NIC_DEVICE *NicDevice
)
{
  UINT32              CmdStatus;
  EFI_USB_IO_PROTOCOL *UsbIo;
  UINT64              IntData = 0;
  UINTN               IntDataLeng = 8;
  EFI_STATUS          Status;

  //
  // Issue the command
  //
  UsbIo = NicDevice->UsbIo;
  Status = UsbIo->UsbSyncInterruptTransfer(UsbIo,
                                        NicDevice->InterruptEndpoint,
                                        &IntData,
                                        &IntDataLeng,
                                        USB_BUS_TIMEOUT,
                                        &CmdStatus);
  DEBUG ((DEBUG_INFO, "  [%a:%d -> %a] %r, %r, D: %08x, L: %d\n", __FILE_NAME__, DEBUG_LINE_NUMBER, __func__, Status, CmdStatus, IntData, IntDataLeng));

  if (EFI_ERROR(Status) || EFI_ERROR(CmdStatus) || 0 == IntDataLeng) {
      return FALSE;
  }
  return (IntData & INT_ENP_PHY_INT_) ? FALSE : TRUE;
}

EFI_STATUS
Smsc95xxBulkIn(
  IN NIC_DEVICE *NicDevice
)
{
  EFI_STATUS          Status = EFI_DEVICE_ERROR;
  EFI_USB_IO_PROTOCOL *UsbIo;
  UINTN               LengthInBytes = 0;
  UINT32              TransferStatus = 0;

  UsbIo = NicDevice->UsbIo;

  while (LengthInBytes < USB_MAX_BULKIN_SIZE)
  {
    UINT8 *TmpAddr = ((UINT8 *)NicDevice->BulkInbuf) + LengthInBytes;
    UINTN TmpLen = USB_MAX_BULKIN_SIZE - LengthInBytes;

    Status = UsbIo->UsbBulkTransfer(UsbIo,
                                    NicDevice->BulkInEndpoint,
                                    TmpAddr,
                                    &TmpLen,
                                    BULKIN_TIMEOUT,
                                    &TransferStatus);

    if (!EFI_ERROR(Status) && !EFI_ERROR(TransferStatus)) {
      LengthInBytes += TmpLen;
      // DEBUG ((DEBUG_INFO, "  [%a:%d -> %a] LengthInBytes: %d, + %d\n", __FILE_NAME__, DEBUG_LINE_NUMBER, __func__, LengthInBytes, TmpLen));
      if (LengthInBytes >= 4) {
        if (LengthInBytes >= NicDevice->BulkInbuf->Length + 4) {
          // ETHERNET_HEADER *EthHead = (ETHERNET_HEADER *)NicDevice->BulkInbuf->Data;
          // if (EthHead->Type != 0x0608 // ARP
          //  && EthHead->Type != 0x0008 // IPv4
          //  )
          // {
          //   UINT32 len = NicDevice->BulkInbuf->Length;
          //   for (UINT32 i = 0; i < len; i++)
          //   {
          //     if (i < 20) {
          //       DEBUG ((DEBUG_INFO, "0x%02x ", NicDevice->BulkInbuf->Data[i]));
          //     }
          //     else if (i == 20) {
          //       DEBUG ((DEBUG_INFO, "... "));
          //     }
          //   }
          //   DEBUG ((DEBUG_INFO, "\n"));
          // }
          // DEBUG ((DEBUG_INFO, "BulkIn->EEEE: 0x%04x, Length: %d\n", NicDevice->BulkInbuf->EEEE, NicDevice->BulkInbuf->Length));
          // if (NicDevice->BulkInbuf->EEEE != 0x2420) {
          //   Status = EFI_INVALID_PARAMETER;
          // }
          goto done;
        }
      }
      if (TmpLen == 0) {
        Status = EFI_NOT_READY;
        goto done;
      }
    } else {
        Status = EFI_NOT_READY;
        goto done;
    }
  }

done:
// no_pkt:
  return Status;
}
