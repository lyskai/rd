// Copyright 2024 Qualcomm Technologies, Inc. All rights reserved.
// Confidential & Proprietary.
#include "ride/hal/Buffer.hpp"
#include "ride/hal/BufferManager.hpp"
#include "ride/hal/Types.hpp"

namespace ride
{
namespace hal
{

static uint32_t s_rideHalTensorTypeToDataSize[RIDE_HAL_TENSOR_TYPE_MAX] = {
        sizeof( int8_t ),   /* RIDE_HAL_TEBSOR_TYPE_INT8 */
        sizeof( int16_t ),  /* RIDE_HAL_TEBSOR_TYPE_INT16 */
        sizeof( int32_t ),  /* RIDE_HAL_TEBSOR_TYPE_INT32 */
        sizeof( uint8_t ),  /* RIDE_HAL_TEBSOR_TYPE_UINT8 */
        sizeof( uint16_t ), /* RIDE_HAL_TEBSOR_TYPE_UINT16 */
        sizeof( uint32_t ), /* RIDE_HAL_TEBSOR_TYPE_UINT32 */
        2,                  /* RIDE_HAL_TEBSOR_TYPE_FLOAT16 */
        sizeof( float )     /* RIDE_HAL_TEBSOR_TYPE_FLOAT32 */
};

RideHalError_e RideHal_SharedBuffer::Allocate( const RideHal_TensorProps_t *pTensorProps,
                                               RideHal_BufferUsage_e usage,
                                               RideHal_BufferFlags_t flags )
{
    RideHalError_e ret = RIDE_HAL_ERROR_NONE;
    size_t size = 1;
    uint32_t i = 0;

    if ( nullptr == pTensorProps )
    {
        ret = RIDE_HAL_ERROR_NULL_PTR;
    }
    else if ( ( pTensorProps->numDims > RIDE_HAL_NUM_DIMS ) ||
              ( pTensorProps->type >= RIDE_HAL_TENSOR_TYPE_MAX ) )
    {
        ret = RIDE_HAL_ERROR_BAD_ARGUMENTS;
    }
    else if ( nullptr != this->buffer.pData )
    {
        ret = RIDE_HAL_ERROR_EXISTS;
    }
    else
    {
        /* check each dimension is reasonable */
        for ( i = 0; ( i < pTensorProps->numDims ) && ( RIDE_HAL_ERROR_NONE == ret ); i++ )
        {
            if ( 0 == pTensorProps->dims[i] )
            {
                ret = RIDE_HAL_ERROR_BAD_ARGUMENTS;
            }
        }
    }

    if ( RIDE_HAL_ERROR_NONE == ret )
    {
        this->tensorProps = *pTensorProps;
        for ( i = 0; i < pTensorProps->numDims; i++ )
        {
            size *= pTensorProps->dims[i];
        }
        size *= s_rideHalTensorTypeToDataSize[pTensorProps->type];
    }

    if ( RIDE_HAL_ERROR_NONE == ret )
    {
        this->type = RIDE_HAL_BUFFER_TYPE_TENSOR;
        ret = Allocate( size, usage, flags );
    }

    return ret;
}

}   // namespace hal
}   // namespace ride
