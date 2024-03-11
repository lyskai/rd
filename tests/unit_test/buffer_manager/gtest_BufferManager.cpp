// Copyright 2024 Qualcomm Technologies, Inc. All rights reserved.
// Confidential & Proprietary - Qualcomm Technologies, Inc. ("QTI")

#include "gtest/gtest.h"
#include <stdio.h>

#include "ride/hal/Types.hpp"

using namespace ride::hal;

TEST( BufferManager, SANITY_ImageAllocateByWHF )
{
    RideHal_SharedBuffer_t sharedBuffer;
    RideHal_SharedBuffer_t sharedBufferM;

    /* testing allocate image for UYVY */
    auto ret = sharedBuffer.Allocate( 3840, 2160, RIDE_HAL_IMAGE_FORMAT_UYVY );
    ASSERT_EQ( RIDE_HAL_ERROR_NONE, ret );
    ASSERT_NE( nullptr, sharedBuffer.data() );
    ASSERT_EQ( 0, sharedBuffer.offset );
    std::generate( (uint8_t *) sharedBuffer.data(),
                   (uint8_t *) sharedBuffer.data() + sharedBuffer.size, std::rand );
    ASSERT_EQ( sharedBuffer.buffer.size, sharedBuffer.size );
    ASSERT_LE( 3840 * 2160 * 2, sharedBuffer.size );
    ASSERT_EQ( 1, sharedBuffer.imgProps.numPlanes );
    ASSERT_LE( 3840 * 2, sharedBuffer.imgProps.stride[0] );
    ASSERT_LE( 2160, sharedBuffer.imgProps.actualHeight[0] );
    ret = sharedBuffer.Free();
    ASSERT_EQ( RIDE_HAL_ERROR_NONE, ret );

    /* testing allocate image for NV12 */
    ret = sharedBuffer.Allocate( 3840, 2160, RIDE_HAL_IMAGE_FORMAT_NV12 );
    ASSERT_EQ( RIDE_HAL_ERROR_NONE, ret );
    ASSERT_NE( nullptr, sharedBuffer.data() );
    ASSERT_EQ( 0, sharedBuffer.offset );
    std::generate( (uint8_t *) sharedBuffer.data(),
                   (uint8_t *) sharedBuffer.data() + sharedBuffer.size, std::rand );
    ASSERT_EQ( sharedBuffer.buffer.size, sharedBuffer.size );
    ASSERT_LE( 3840 * 2160 * 3 / 2, sharedBuffer.size );
    ASSERT_EQ( 2, sharedBuffer.imgProps.numPlanes );
    ASSERT_LE( 3840, sharedBuffer.imgProps.stride[0] );
    ASSERT_LE( 2160, sharedBuffer.imgProps.actualHeight[0] );
    ASSERT_LE( 3840, sharedBuffer.imgProps.stride[1] );
    ASSERT_LE( 2160 / 2, sharedBuffer.imgProps.actualHeight[1] );
    ret = sharedBuffer.Free();
    ASSERT_EQ( RIDE_HAL_ERROR_NONE, ret );

    /* testing allocate image for RGB */
    ret = sharedBuffer.Allocate( 1024, 768, RIDE_HAL_IMAGE_FORMAT_RGB888 );
    ASSERT_EQ( RIDE_HAL_ERROR_NONE, ret );
    ASSERT_NE( nullptr, sharedBuffer.data() );
    ASSERT_EQ( 0, sharedBuffer.offset );
    ASSERT_EQ( sharedBuffer.buffer.size, sharedBuffer.size );
    ASSERT_LE( 1024 * 768 * 3, sharedBuffer.size );
    ASSERT_EQ( 1, sharedBuffer.imgProps.numPlanes );
    ASSERT_LE( 1024, sharedBuffer.imgProps.stride[0] );
    ASSERT_LE( 768, sharedBuffer.imgProps.actualHeight[0] );
    ret = sharedBuffer.Free();
    ASSERT_EQ( RIDE_HAL_ERROR_NONE, ret );

    /* testing allocate batched image for RGB */
    ret = sharedBuffer.Allocate( 7, 1024, 768, RIDE_HAL_IMAGE_FORMAT_RGB888 );
    ASSERT_EQ( RIDE_HAL_ERROR_NONE, ret );
    ASSERT_NE( nullptr, sharedBuffer.data() );
    ASSERT_EQ( 0, sharedBuffer.offset );
    std::generate( (uint8_t *) sharedBuffer.data(),
                   (uint8_t *) sharedBuffer.data() + sharedBuffer.size, std::rand );
    ASSERT_EQ( sharedBuffer.buffer.size, sharedBuffer.size );
    ASSERT_LE( 1024 * 768 * 3 * 7, sharedBuffer.size );
    ASSERT_EQ( 1, sharedBuffer.imgProps.numPlanes );
    ASSERT_LE( 1024, sharedBuffer.imgProps.stride[0] );
    ASSERT_LE( 768, sharedBuffer.imgProps.actualHeight[0] );
    /* testing get the middle batch of the shared image */
    ret = sharedBuffer.GetSharedBuffer( &sharedBufferM, 3 );
    ASSERT_EQ( RIDE_HAL_ERROR_NONE, ret );
    ASSERT_EQ( (uint8_t *) sharedBuffer.buffer.pData + sharedBufferM.size * 3,
               sharedBufferM.data() );
    ASSERT_EQ( sharedBufferM.size * 3, sharedBufferM.offset );
    ASSERT_EQ( sharedBufferM.buffer.size / 7, sharedBufferM.size );
    ASSERT_LE( 1024 * 768 * 3, sharedBufferM.size );
    ASSERT_EQ( 1, sharedBufferM.imgProps.numPlanes );
    ASSERT_LE( 1024, sharedBufferM.imgProps.stride[0] );
    ASSERT_LE( 768, sharedBufferM.imgProps.actualHeight[0] );

    ret = sharedBuffer.Free();
    ASSERT_EQ( RIDE_HAL_ERROR_NONE, ret );
}

TEST( BufferManager, SANITY_ImageAllocateByProps )
{
    RideHal_SharedBuffer_t sharedBuffer;
    RideHal_ImageProps_t imgProp;

    imgProp.format = RIDE_HAL_IMAGE_FORMAT_UYVY;
    imgProp.batchSize = 1;
    imgProp.width = 3840;
    imgProp.height = 2160;
    imgProp.stride[0] = 3840 * 2;
    imgProp.actualHeight[0] = 2160;
    imgProp.numPlanes = 1;
    imgProp.extraPadding = 0;
    auto ret = sharedBuffer.Allocate( &imgProp );
    ASSERT_EQ( RIDE_HAL_ERROR_NONE, ret );
    ASSERT_NE( nullptr, sharedBuffer.data() );
    ASSERT_EQ( 0, sharedBuffer.offset );
    std::generate( (uint8_t *) sharedBuffer.data(),
                   (uint8_t *) sharedBuffer.data() + sharedBuffer.size, std::rand );
    ASSERT_EQ( sharedBuffer.buffer.size, sharedBuffer.size );
    ASSERT_EQ( 3840 * 2160 * 2, sharedBuffer.size );
    ASSERT_EQ( 1, sharedBuffer.imgProps.numPlanes );
    ASSERT_EQ( 3840 * 2, sharedBuffer.imgProps.stride[0] );
    ASSERT_EQ( 2160, sharedBuffer.imgProps.actualHeight[0] );
    ret = sharedBuffer.Free();
    ASSERT_EQ( RIDE_HAL_ERROR_NONE, ret );
}

TEST( BufferManager, SANITY_CompressedImageAllocateByProps )
{
    RideHal_SharedBuffer_t sharedBuffer;
    RideHal_ImageProps_t imgProp;

    imgProp.format = RIDE_HAL_IMAGE_FORMAT_COMPRESSED_H265;
    imgProp.batchSize = 1;
    imgProp.width = 3840;
    imgProp.height = 2160;
    imgProp.numPlanes = 0;
    imgProp.compressedSize = 1024 * 64;
    auto ret = sharedBuffer.Allocate( &imgProp );
    ASSERT_EQ( RIDE_HAL_ERROR_NONE, ret );
    ASSERT_NE( nullptr, sharedBuffer.data() );
    ASSERT_EQ( 0, sharedBuffer.offset );
    std::generate( (uint8_t *) sharedBuffer.data(),
                   (uint8_t *) sharedBuffer.data() + sharedBuffer.size, std::rand );
    ASSERT_EQ( sharedBuffer.buffer.size, sharedBuffer.size );
    ASSERT_EQ( 1024 * 64, sharedBuffer.size );
    ASSERT_EQ( 0, sharedBuffer.imgProps.numPlanes );
    ret = sharedBuffer.Free();
    ASSERT_EQ( RIDE_HAL_ERROR_NONE, ret );
}

TEST( BufferManager, SANITY_TensorAllocate )
{
    RideHal_SharedBuffer_t sharedBuffer;
    RideHal_TensorProps_t tensorProp = { RIDE_HAL_TENSOR_TYPE_UINT8, { 1, 128, 128, 10 }, 4 };

    auto ret = sharedBuffer.Allocate( &tensorProp );
    ASSERT_EQ( RIDE_HAL_ERROR_NONE, ret );
    ASSERT_NE( nullptr, sharedBuffer.data() );
    ASSERT_EQ( 0, sharedBuffer.offset );
    std::generate( (uint8_t *) sharedBuffer.data(),
                   (uint8_t *) sharedBuffer.data() + sharedBuffer.size, std::rand );
    ASSERT_EQ( sharedBuffer.buffer.size, sharedBuffer.size );
    ASSERT_EQ( 1 * 128 * 128 * 10, sharedBuffer.size );
    ASSERT_EQ( 4, sharedBuffer.tensorProps.numDims );
    ret = sharedBuffer.Free();
    ASSERT_EQ( RIDE_HAL_ERROR_NONE, ret );
}

#ifndef GTEST_RIDEHAL
int main( int argc, char **argv )
{
    ::testing::InitGoogleTest( &argc, argv );
    int nVal = RUN_ALL_TESTS();
    return nVal;
}
#endif
