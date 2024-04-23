// Copyright 2024 Qualcomm Technologies, Inc. All rights reserved.
// Confidential & Proprietary - Qualcomm Technologies, Inc. ("QTI")

#include "gtest/gtest.h"
#include <stdio.h>

#include "ridehal/component/ComponentIF.hpp"

using namespace ridehal::common;
using namespace ridehal::component;

typedef struct
{
    int a;
} ComponentIFTest_Config_t;

class ComponentIFTest : public ComponentIF
{
public:
    ComponentIFTest() {}
    ~ComponentIFTest() {}
    RideHalError_e Init( const char *pName, const ComponentIFTest_Config_t *pConfig,
                         Logger_Level_e level = LOGGER_LEVEL_ERROR )
    {
        RideHalError_e ret = RIDEHAL_ERROR_NONE;

        ret = ComponentIF::Init( pName );
        if ( RIDEHAL_ERROR_NONE == ret )
        {
            // DO real initialize using pConfig.
        }

        if ( RIDEHAL_ERROR_NONE == ret )
        {
            m_state = RIDEHAL_COMPONENT_STATE_READY;
        }

        return ret;
    }

    RideHalError_e Start()
    {
        RideHalError_e ret = RIDEHAL_ERROR_NONE;

        if ( RIDEHAL_COMPONENT_STATE_READY != m_state )
        {
            ret = RIDEHAL_ERROR_BAD_STATE;
        }

        if ( RIDEHAL_ERROR_NONE == ret )
        {
            // DO start
        }

        if ( RIDEHAL_ERROR_NONE == ret )
        {
            m_state = RIDEHAL_COMPONENT_STATE_RUNNING;
        }

        return ret;
    }


    RideHalError_e Stop()
    {
        RideHalError_e ret = RIDEHAL_ERROR_NONE;

        if ( RIDEHAL_COMPONENT_STATE_RUNNING != m_state )
        {
            ret = RIDEHAL_ERROR_BAD_STATE;
        }

        if ( RIDEHAL_ERROR_NONE == ret )
        {
            // DO stop
        }

        if ( RIDEHAL_ERROR_NONE == ret )
        {
            m_state = RIDEHAL_COMPONENT_STATE_READY;
        }

        return ret;
    }

    RideHalError_e Deinit()
    {
        RideHalError_e ret = RIDEHAL_ERROR_NONE;

        if ( RIDEHAL_COMPONENT_STATE_READY != m_state )
        {
            ret = RIDEHAL_ERROR_BAD_STATE;
        }

        if ( RIDEHAL_ERROR_NONE == ret )
        {
            // DO deinit
        }

        if ( RIDEHAL_ERROR_NONE == ret )
        {
            ret = ComponentIF::Deinit();
        }

        return ret;
    }
};

TEST( ComponentIF, SANITY_ComponentIF )
{
    ComponentIFTest cifTest;
    ComponentIFTest_Config_t config = { 10 };
    RideHalError_e ret;

    for ( int i = 0; i < 3; i++ )
    {
        ASSERT_EQ( RIDEHAL_COMPONENT_STATE_INITIAL, cifTest.GetState() );

        ret = cifTest.Init( "test", &config );
        ASSERT_EQ( RIDEHAL_ERROR_NONE, ret );
        ASSERT_EQ( RIDEHAL_COMPONENT_STATE_READY, cifTest.GetState() );

        ret = cifTest.Start();
        ASSERT_EQ( RIDEHAL_ERROR_NONE, ret );
        ASSERT_EQ( RIDEHAL_COMPONENT_STATE_RUNNING, cifTest.GetState() );

        ret = cifTest.Stop();
        ASSERT_EQ( RIDEHAL_ERROR_NONE, ret );
        ASSERT_EQ( RIDEHAL_COMPONENT_STATE_READY, cifTest.GetState() );

        ret = cifTest.Deinit();
        ASSERT_EQ( RIDEHAL_ERROR_NONE, ret );
        ASSERT_EQ( RIDEHAL_COMPONENT_STATE_INITIAL, cifTest.GetState() );
    }
}

#ifndef GTEST_RIDEHAL
int main( int argc, char **argv )
{
    ::testing::InitGoogleTest( &argc, argv );
    int nVal = RUN_ALL_TESTS();
    return nVal;
}
#endif