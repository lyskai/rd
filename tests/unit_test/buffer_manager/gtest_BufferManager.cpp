// Copyright 2024 Qualcomm Technologies, Inc. All rights reserved.
// Confidential & Proprietary - Qualcomm Technologies, Inc. ("QTI")

#include "gtest/gtest.h"
#include <stdio.h>

#include "ride/hal/Image.hpp"
#include "ride/hal/Tensor.hpp"

using namespace ride::hal;
using namespace ride::hal::memory;

TEST( BufferManager, SANITY_ImageAllocateByWHF )
{
    Image img;
    RideHal_SharedBuffer_t sharedBuffer;
    RideHal_SharedBuffer_t sharedBufferM;

    /* testing allocate image for UYVY */
    auto ret = img.Allocate( 3840, 2160, RIDE_HAL_IMAGE_FORMAT_UYVY );
    ASSERT_EQ( RIDE_HAL_ERROR_NONE, ret );
    ret = img.GetSharedBuffer( &sharedBuffer );
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
    ret = img.Free();
    ASSERT_EQ( RIDE_HAL_ERROR_NONE, ret );

    /* testing allocate image for NV12 */
    ret = img.Allocate( 3840, 2160, RIDE_HAL_IMAGE_FORMAT_NV12 );
    ASSERT_EQ( RIDE_HAL_ERROR_NONE, ret );
    ret = img.GetSharedBuffer( &sharedBuffer );
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
    ret = img.Free();
    ASSERT_EQ( RIDE_HAL_ERROR_NONE, ret );

    /* testing allocate image for RGB */
    ret = img.Allocate( 1024, 768, RIDE_HAL_IMAGE_FORMAT_RGB888 );
    ASSERT_EQ( RIDE_HAL_ERROR_NONE, ret );
    ret = img.GetSharedBuffer( &sharedBuffer );
    ASSERT_EQ( RIDE_HAL_ERROR_NONE, ret );
    ASSERT_NE( nullptr, sharedBuffer.data() );
    ASSERT_EQ( 0, sharedBuffer.offset );
    ASSERT_EQ( sharedBuffer.buffer.size, sharedBuffer.size );
    ASSERT_LE( 1024 * 768 * 3, sharedBuffer.size );
    ASSERT_EQ( 1, sharedBuffer.imgProps.numPlanes );
    ASSERT_LE( 1024, sharedBuffer.imgProps.stride[0] );
    ASSERT_LE( 768, sharedBuffer.imgProps.actualHeight[0] );
    ret = img.Free();
    ASSERT_EQ( RIDE_HAL_ERROR_NONE, ret );

    /* testing allocate batched image for RGB */
    ret = img.Allocate( 7, 1024, 768, RIDE_HAL_IMAGE_FORMAT_RGB888 );
    ASSERT_EQ( RIDE_HAL_ERROR_NONE, ret );
    ret = img.GetSharedBuffer( &sharedBuffer );
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
    ret = img.GetSharedBuffer( &sharedBufferM, 3 );
    ASSERT_EQ( RIDE_HAL_ERROR_NONE, ret );
    ASSERT_EQ( (uint8_t *) sharedBuffer.buffer.pData + sharedBufferM.size * 3,
               sharedBufferM.data() );
    ASSERT_EQ( sharedBufferM.size * 3, sharedBufferM.offset );
    ASSERT_EQ( sharedBufferM.buffer.size / 7, sharedBufferM.size );
    ASSERT_LE( 1024 * 768 * 3, sharedBufferM.size );
    ASSERT_EQ( 1, sharedBufferM.imgProps.numPlanes );
    ASSERT_LE( 1024, sharedBufferM.imgProps.stride[0] );
    ASSERT_LE( 768, sharedBufferM.imgProps.actualHeight[0] );

    ret = img.Free();
    ASSERT_EQ( RIDE_HAL_ERROR_NONE, ret );
}

TEST( BufferManager, SANITY_ImageAllocateByProps )
{
    Image img;
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
    auto ret = img.Allocate( &imgProp );
    ASSERT_EQ( RIDE_HAL_ERROR_NONE, ret );
    ret = img.GetSharedBuffer( &sharedBuffer );
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
    ret = img.Free();
    ASSERT_EQ( RIDE_HAL_ERROR_NONE, ret );
}

TEST( BufferManager, SANITY_TensorAllocate )
{
    Tensor tensor;
    RideHal_SharedBuffer_t sharedBuffer;
    RideHal_TensorProps_t tensorProp = { RIDE_HAL_TEBSOR_TYPE_UINT8, { 1, 128, 128, 10 }, 4 };

    auto ret = tensor.Allocate( &tensorProp );
    ASSERT_EQ( RIDE_HAL_ERROR_NONE, ret );
    ret = tensor.GetSharedBuffer( &sharedBuffer );
    ASSERT_EQ( RIDE_HAL_ERROR_NONE, ret );
    ASSERT_NE( nullptr, sharedBuffer.data() );
    ASSERT_EQ( 0, sharedBuffer.offset );
    std::generate( (uint8_t *) sharedBuffer.data(),
                   (uint8_t *) sharedBuffer.data() + sharedBuffer.size, std::rand );
    ASSERT_EQ( sharedBuffer.buffer.size, sharedBuffer.size );
    ASSERT_EQ( 1 * 128 * 128 * 10, sharedBuffer.size );
    ASSERT_EQ( 4, sharedBuffer.tensorProps.numDims );
    ret = tensor.Free();
    ASSERT_EQ( RIDE_HAL_ERROR_NONE, ret );
}

int main( int argc, char **argv )
{
    ::testing::InitGoogleTest( &argc, argv );
    int nVal = RUN_ALL_TESTS();
    return nVal;
}
