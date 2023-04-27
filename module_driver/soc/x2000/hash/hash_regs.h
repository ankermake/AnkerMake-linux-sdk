#ifndef _HASH_REGS_H
#define _HASH_REGS_H

#define HSCR                             0x0000
#define HSSR                             0x0004
#define HSINTM                           0x0008
#define HSSA                             0x000c
#define HSTC                             0x0010
#define HSDI                             0x0014
#define HSDO                             0x0018


#define HSCR_PS                          28, 31
#define HSCR_DORVS                       8, 8
#define HSCR_DIRVS                       7, 7
#define HSCR_DMAS                        6, 6
#define HSCR_DMAE                        5, 5
#define HSCR_INIT                        4, 4
#define HSCR_SEL                         1, 3
#define HSCR_EN                          0, 0

#define HSSR_MRD                         1, 1
#define HSSR_ORD                         0, 0

#define HSINTM_MR_INT_M                  1, 1
#define HSINTM_OR_INT_M                  0, 0

#define HSSA_SA                          0, 31

#define HSTC_TC                          0, 31

#define HSDI_DIN                         0, 31

#define HSDO_DOUT                        0, 31


#endif /* X2000 HASH REGS */
