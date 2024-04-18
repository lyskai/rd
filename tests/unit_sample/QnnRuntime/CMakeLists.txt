# Copyright 2024 Qualcomm Technologies, Inc. All rights reserved.
# Confidential & Proprietary - Qualcomm Technologies, Inc. ("QTI")

if( NOT TARGET RideHalQnnRuntime )
  message(STATUS "skip building of qnn_test")
else()
  add_executable( qnn_test qnn_test.cpp )
  target_include_directories( qnn_test PUBLIC ${HEADERS_DIR} )
  target_link_libraries( qnn_test gtest RideHalComponentCommon RideHalQnnRuntime )
  install(TARGETS qnn_test DESTINATION bin)
endif()
