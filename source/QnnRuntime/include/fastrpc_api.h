/*
 * Copyright (c) 2020 Qualcomm Technologies, Inc.
 * All Rights Reserved.
 * Confidential and Proprietary - Qualcomm Technologies, Inc
 */

#ifndef __ADSPRPC_QNX_H__
#define __ADSPRPC_QNX_H__

#define ADSP_DOMAIN_ID 0
#define MDSP_DOMAIN_ID 1
#define SDSP_DOMAIN_ID 2
#define CDSP_DOMAIN_ID 3
#define CDSP1_DOMAIN_ID 4

#define AUDIO_PD 0
#define DYNAMIC_PD 1
#define GUEST_OS 2

int fastrpc_qnx_init( int domain_id, int pd );
int fastrpc_qnx_deinit_domain( int domain_id );

#endif /* __ADSPRPC_QNX_H__ */
