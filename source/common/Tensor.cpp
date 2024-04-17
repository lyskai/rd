// Copyright 2024 Qualcomm Technologies, Inc. All rights reserved.
// Confidential & Proprietary.
#include "ridehal/common/BufferManager.hpp"
#include "ridehal/common/SharedBuffer.hpp"

namespace ridehal
{
namespace common
{
#define SIZE_OF_FLOAT16 2
static uint32_t s_rideHalTensorTypeToDataSize[RIDE_HAL_TENSOR_TYPE_MAX] = {
        sizeof( int8_t ),  /* RIDEHAL_TENSOR_TYPE_INT_8 */
        sizeof( int16_t ), /* RIDEHAL_TENSOR_TYPE_INT_16 */
        sizeof( int32_t ), /* RIDEHAL_TENSOR_TYPE_INT_32 */
        sizeof( int64_t ), /* RIDEHAL_TENSOR_TYPE_INT_64 */

        sizeof( uint8_t ),  /* RIDEHAL_TENSOR_TYPE_UINT_8 */
        sizeof( uint16_t ), /* RIDEHAL_TENSOR_TYPE_UINT_16 */
        sizeof( uint32_t ), /* RIDEHAL_TENSOR_TYPE_UINT_32 */
        sizeof( uint64_t ), /* RIDEHAL_TENSOR_TYPE_UINT_64 */

        SIZE_OF_FLOAT16,  /* RIDEHAL_TENSOR_TYPE_FLOAT_16 */
        sizeof( float ),  /* RIDEHAL_TENSOR_TYPE_FLOAT_32 */
        sizeof( double ), /* RIDEHAL_TENSOR_TYPE_FLOAT_64 */

        sizeof( int8_t ),  /* RIDEHAL_TENSOR_TYPE_SFIXED_POINT_8 */
        sizeof( int16_t ), /* RIDEHAL_TENSOR_TYPE_SFIXED_POINT_16 */
        sizeof( int32_t ), /* RIDEHAL_TENSOR_TYPE_SFIXED_POINT_32 */

        sizeof( uint8_t ),  /* RIDEHAL_TENSOR_TYPE_UFIXED_POINT_8 */
        sizeof( uint16_t ), /* RIDEHAL_TENSOR_TYPE_UFIXED_POINT_16 */
        sizeof( uint32_t )  /* RIDEHAL_TENSOR_TYPE_UFIXED_POINT_32 */
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
}   // namespace common
}   // namespace ridehal
