#ifndef FADAS__H
#define FADAS__H

/***************************************************************************//**
@brief
   FADAS internal header file

@internal
   Copyright 2020 Qualcomm Technologies, Inc.  All rights reserved.
   Confidential & Proprietary.
*******************************************************************************/

#ifdef __cplusplus
extern "C"
{
#endif

/************************************************************************//**
@details
   Target platform.
****************************************************************************/
typedef enum
{
    /**************************************************************************************
     *  We should never remove or change the order.                                       *
     *  If there is new Implementation, need to add before FADAS_IMPL_MAX.                *
     **************************************************************************************/
    //CPU reference implementation
    FADAS_IMPL_CPU_1 = 0,
    //Neon optimized implementation
    FADAS_IMPL_CPU_2,
    //DSP reference implementation
    FADAS_IMPL_NSP_1,
    //HVX optimized implementation
    FADAS_IMPL_NSP_2,
    //x86 reference implementation
    FADAS_IMPL_X86,
    //default implementation (platform dependent)
    FADAS_IMPL_DEFAULT,
    FADAS_IMPL_MAX = 0x7FFFFFFF
} FADAS_IMPL;

extern FADAS_IMPL g_mode;

/************************************************************************//**
@brief
   Set the target for FastADAS.

@return
   "true" if successful.
****************************************************************************/
FADAS_API bool
FadasSetMode(FADAS_IMPL mode);

#ifdef __cplusplus
}
#endif

#endif /* FADAS__H */
