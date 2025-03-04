if( TARGET RideHal )
  return()
endif()

string( REPLACE "/lib/cmake/ridehal" "" _ridehal_prefix ${CMAKE_CURRENT_LIST_DIR} )
set( _ridehal_include_dir
  ${_ridehal_prefix}/include
  ${_ridehal_prefix}/include/ridehal/libs/FadasIface
  ${_ridehal_prefix}/include/ridehal/libs/OpenclIface
  ${_ridehal_prefix}/include/ridehal/libs/VideoCodec )

set( _ridehal_defines )

if( DEFINED ENV{QNN_SDK_ROOT} )
    set( QNN_SDK_ROOT $ENV{QNN_SDK_ROOT} )
endif()

if( NOT DEFINED QNN_SDK_ROOT )
  message( WARNING "QNN_SDK_ROOT is not defined" )
else()
  if (EXISTS "${QNN_SDK_ROOT}/examples/QNN/SampleApp/SampleApp")
    set( QNN_SAMPLEAPP_DIR ${QNN_SDK_ROOT}/examples/QNN/SampleApp/SampleApp )
  else()
    set( QNN_SAMPLEAPP_DIR ${QNN_SDK_ROOT}/examples/QNN/SampleApp )
  endif()
  list( APPEND _ridehal_include_dir
    ${QNN_SAMPLEAPP_DIR}/src
    ${QNN_SAMPLEAPP_DIR}/src/Log
    ${QNN_SAMPLEAPP_DIR}/src/PAL/include
    ${QNN_SAMPLEAPP_DIR}/src/Utils
    ${QNN_SAMPLEAPP_DIR}/src/WrapperUtils
    ${QNN_SDK_ROOT}/include/QNN
)
endif()

if( "${CMAKE_SYSTEM_NAME}" STREQUAL "Linux" )
    list( APPEND _ridehal_include_dir ${CMAKE_SYSROOT}/usr/include/mm-osal/include )
    list( APPEND _ridehal_defines _VIDC_LRH_LINUX_ CL_TARGET_OPENCL_VERSION=200 )
    list( APPEND _ridehal_include_dir ${CMAKE_SYSROOT}/usr/include/drm )
    list( APPEND _ridehal_include_dir ${CMAKE_SYSROOT}/usr/include/libdrm )
    list( APPEND _ridehal_include_dir ${CMAKE_SYSROOT}/usr/include/amss/multimedia/fadas )
else()
    list( APPEND _ridehal_defines CL_TARGET_OPENCL_VERSION=300 )
endif()

set( _ridehal_library_dir ${_ridehal_prefix}/lib )

set( _ridehal_libs
  ${_ridehal_library_dir}/libRideHal.so
  ${_ridehal_library_dir}/libFadasIface.so
  ${_ridehal_library_dir}/libFadasIfaceStub.so
  ${_ridehal_library_dir}/libOpenclIface.so
  ${_ridehal_library_dir}/libRideHalVideoCodec.so )

if( "${CMAKE_SYSTEM_NAME}" STREQUAL "QNX" )
    list( APPEND _ridehal_libs
      qcxclient qcxosal
      c2d30 OSUser GSLUser
      fadas
      evaEpl evaPlatform devioClient )
endif()

add_library( RideHal INTERFACE IMPORTED )
target_include_directories( RideHal INTERFACE ${_ridehal_include_dir} )
target_link_libraries( RideHal INTERFACE ${_ridehal_libs} )
target_link_directories( RideHal INTERFACE ${_ridehal_library_dir} )
target_compile_definitions( RideHal INTERFACE ${_ridehal_defines} )

set( RIDEHAL_VERSION "1.8.0" )
set( RIDEHAL_LIBRARIES RideHal )

message( STATUS "RideHal version: ${RideHal_VERSION} : ${_ridehal_prefix}" )

