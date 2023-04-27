/*
 * Copyright (C) 2020 Ingenic Semiconductor Co., Ltd.
 *
 * I2C adapter driver for the Ingenic I2C controller
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#ifndef __X1830_I2C_REGS_H__
#define __X1830_I2C_REGS_H__

#define SMB_CON                         0x0000
#define SMB_TAR                         0x0004
#define SMB_SAR                         0x0008
#define SMB_DC                          0x0010
#define SMB_SHCNT                       0x0014
#define SMB_SLCNT                       0x0018
#define SMB_FHCNT                       0x001C
#define SMB_FLCNT                       0x0020
#define SMB_INTST                       0x002C
#define SMB_INTM                        0x0030
#define SMB_RAW_INTR_STAT               0x0034
#define SMB_RXTL                        0x0038
#define SMB_TXTL                        0x003C
#define SMB_CINT                        0x0040
#define SMB_CRXUF                       0x0044
#define SMB_CRXOF                       0x0048
#define SMB_CTXOF                       0x004C
#define SMB_CRXREQ                      0x0050
#define SMB_CTXABT                      0x0054
#define SMB_CRXDN                       0x0058
#define SMB_CACT                        0x005C
#define SMB_CSTP                        0x0060
#define SMB_CSTT                        0x0064
#define SMB_CGC                         0x0068
#define SMB_ENABLE                      0x006C
#define SMB_ST                          0x0070
#define SMB_TXFLR                       0x0074
#define SMB_RXFLR                       0X0078
#define SMB_SDAHD                       0x007C
#define SMB_ABTSRC                      0x0080
#define SMB_DMACR                       0x0088
#define SMB_DMATDLR                     0x008C
#define SMB_DMARDLR                     0x0090
#define SMB_SDASU                       0x0094
#define SMB_ACKGC                       0x0098
#define SMB_ENBST                       0x009C
#define SMB_FLT                         0x00A0

#define SMBCON_SLVDIS                   6, 6
#define SMBCON_RESTART                  5, 5
#define SMBCON_MATP                     4, 4
#define SMBCON_SATP                     3, 3
#define SMBCON_SPEED                    1, 2
#define SMBCON_MD                       0, 0

#define SMBTAR_MATP                     12, 12
#define SMBTAR_SPECIAL                  11, 11
#define SMBTAR_GC_OR_START              10, 10
#define SMBTAR_SMBTAR                   0, 9

#define SMBSAR                          0, 9

#define SMBDC_RESTART                   10, 10
#define SMBDC_STOP                      9, 9
#define SMBDC_CMD                       8, 8
#define SMBDC_DAT                       0, 7

#define SMBSHCNT                        0, 15
#define SMBSLCNT                        0, 15
#define SMBFHCNT                        0, 15
#define SMBFLCNT                        0, 15

#define SMBINTST_IGC                    11, 11
#define SMBINTST_ISTT                   10, 10
#define SMBINTST_ISTP                   9, 9
#define SMBINTST_IACT                   8, 8
#define SMBINTST_RXDN                   7, 7
#define SMBINTST_TXABT                  6, 6
#define SMBINTST_RDREQ                  5, 5
#define SMBINTST_TXEMP                  4, 4
#define SMBINTST_TXOF                   3, 3
#define SMBINTST_RXFL                   2, 2
#define SMBINTST_RXOF                   1, 1
#define SMBINTST_RXUF                   0, 0

#define SMBINTM_MIGC                    11, 11
#define SMBINTM_MISTT                   10, 10
#define SMBINTM_MISTP                   9, 9
#define SMBINTM_MIACT                   8, 8
#define SMBINTM_MRXDN                   7, 7
#define SMBINTM_MTXABT                  6, 6
#define SMBINTM_MRDREQ                  5, 5
#define SMBINTM_MTXEMP                  4, 4
#define SMBINTM_MTXOF                   3, 3
#define SMBINTM_MRXFL                   2, 2
#define SMBINTM_MRXOF                   1, 1
#define SMBINTM_MRXUF                   0, 0

#define SMBRXTL_RXTL                    0, 5
#define SMBTXTL_TXTL                    0, 5

//clear interrupt regs
#define SMBCINT                         0, 0
#define SMBCRXUF                        0, 0
#define SMBCRXOF                        0, 0
#define SMBCTXOF                        0, 0
#define SMBCLRDREQ                      0, 0
#define SMBCTXABT                       0, 0
#define SMBCRXDN                        0, 0
#define SMBCACT                         0, 0
#define SMBCSTP                         0, 0
#define SMBCSTT                         0, 0
#define SMBCGC                          0, 0

#define SMBENABLE_ABORT                 1, 1
#define SMBENABLE_SMBENB                0, 0

#define SMBST_SLVACT                    6, 6
#define SMBST_MSTACT                    5, 5
#define SMBST_RFF                       4, 4
#define SMBST_RFNE                      3, 3
#define SMBST_TFE                       2, 2
#define SMBST_TFNF                      1, 1
#define SMBST_ACT                       0, 0

#define SMBTXFLR                        0, 5
#define SMBRXFLR                        0, 5

#define SMBSDAHD                        0, 15

#define SMBABTSRC_TX_FLUSH_CNT          24, 31
#define SMBABTSRC_USER_ABRT             16, 16
#define SMBABTSRC_SLVRD_INTX            15, 15
#define SMBABTSRC_SLV_ARBLOST           14, 14
#define SMBABTSRC_SLVFLUSH_TXFIFO       13, 13
#define SMBABTSRC_ARB_LOST              12, 12
#define SMBABTSRC_ABRT_MASTER_DIS       11, 11
#define SMBABTSRC_ABRT_10B_RD_NORSTRT   10, 10
#define SMBABTSRC_SBYTE_NORSTRT         9, 9
#define SMBABTSRC_ABRT_HS_NORSTRT       8, 8
#define SMBABTSRC_SBYTE_ACKDET          7, 7
#define SMBABTSRC_ABRT_HS_ACKDET        6, 6
#define SMBABTSRC_ABRT_GCALL_READ       5, 5
#define SMBABTSRC_ABRT_GCALL_NOACK      4, 4
#define SMBABTSRC_ABRT_TXDATA_NOACK     3, 3
#define SMBABTSRC_ABRT_10ADDR2_NOACK    2, 2
#define SMBABTSRC_ABRT_10ADDR1_NOACK    1, 1
#define SMBABTSRC_ABRT_7B_ADDR_NOACK    0, 0

#define SMBDMACR_TDEN                   1, 1
#define SMBDMACR_RDEN                   0, 0

#define SMBDMATDLR                      0, 5

#define SMBSDASU                        0, 7

#define SMBACKGC                        0, 0

#define SMBENBST_SLVRDLST               2, 2
#define SMBENBST_SLVDISB                1, 1
#define SMBENBST_SMBEN                  0, 0

#define SMBFLTCNT                       0, 7

#endif /* __X1830_I2C_REGS_H__ */
