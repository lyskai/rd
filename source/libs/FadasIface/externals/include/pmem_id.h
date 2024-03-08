/*===========================================================================
  Copyright (c) 2018 by Qualcomm Technologies, Incorporated.  All Rights Reserved.
===========================================================================*/
#ifndef _QNX_PMEM_IDS_H_
#define _QNX_PMEM_IDS_H_

/* The attributes of these clients may be stored in the device tree*/
#define MAX_NONSEC_ID                       19
#define PMEM_UNCACHED_ID                    0
#define PMEM_GRAPHICS_FRAMEBUFFER_ID        1
#define PMEM_VIDEO_ID                       2
#define PMEM_MDP_ID                         3
#define PMEM_USB_ID                         4
#define PMEM_CAMERA_ID                      5
#define PMEM_AUDIO_ID                       6
#define PMEM_SMMU_ID                        7
#define PMEM_SPI_ID                         8
#define PMEM_QSEECOM_ID                     9
#define PMEM_I2C_ID                         10
#define PMEM_UART_ID                        11
#define PMEM_PCI_ETH_ID                     12
#define PMEM_QCPE_ID                        13
#define PMEM_DMA_ID                         14
#define PMEM_DSP_ID                         15
#define PMEM_MDSP_FRPC_ID                   16
#define PMEM_MDF_ID                         17
#define PMEM_CVP_ID                         18
#define PMEM_QAIC_ID                        19
/*Additional ids are required please add before this*/
#define PMEM_SECMEM_CP_P_ID                 (MAX_NONSEC_ID + 1)
#define PMEM_SECMEM_CP_NP_ID                (MAX_NONSEC_ID + 2)
#define PMEM_SECMEM_CP_B_ID                 (MAX_NONSEC_ID + 3)
#define PMEM_SEC_SPI_ID                     (MAX_NONSEC_ID + 4)
#define PMEM_MAX_ID                         (MAX_NONSEC_ID + 5)

/* Pseudo pmem id to manage access control on "pmem_map_handle()"*/
#define PMEM_MAPPABLE_ID                    (0xffffffff - 1)
#endif 
