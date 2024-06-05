# Copyright 2024 Qualcomm Technologies, Inc. All rights reserved.
# Confidential & Proprietary - Qualcomm Technologies, Inc. ("QTI")

set( TEST_UTILS_SOURCES
    md5_utils.cpp
)

set( TARGET_LIBRARIES ssl crypto )

add_library( RideHalTestUtils STATIC ${TEST_UTILS_SOURCES} )

set_property( TARGET RideHalTestUtils PROPERTY LINKER_LANGUAGE CXX )
set_property( TARGET RideHalTestUtils PROPERTY POSITION_INDEPENDENT_CODE ON )

target_link_libraries( RideHalTestUtils PUBLIC ${TARGET_LIBRARIES} )

install(TARGETS RideHalTestUtils DESTINATION lib)

add_executable( RideHalMD5 md5_utils.cpp )
target_link_libraries( RideHalMD5 ${TARGET_LIBRARIES} )
target_compile_definitions(RideHalMD5 PUBLIC MD5_UTILS_TEST)
install(TARGETS RideHalMD5 DESTINATION bin)
