/***************************************************************************/ /**
 @file
     fadas.h

 @brief
     Main entry point with common functions and data structures.

 @mainpage
     FastADAS SDK

 @version
     0.9.1

     The release numbering scheme follows conventions in
     <a href="http://www.semver.org/">www.semver.org</a>

 @section Overview
     The Qualcomm FastADAS SDK provides highly runtime optimized and
     state-of-the-art ADAS and autonomy algorithms.  FastADAS is designed to
     allow platform and ADAS systems to operate at peak efficiency and
     minimum latency. Developers can use this SDK to optimize and reduce the
     end-to-end latency of their most critical pipelines to accelerate their
     algorithms and systems. The SDK is a C/C++ programming API composed of
     binary libraries and header files.

 @defgroup misc Miscellaneous

 @details
     Support functions

 @internal
     Copyright (c)2020--2022 Qualcomm Technologies, Inc. All Rights Reserved.
     Confidential and Proprietary - Qualcomm Technologies, Inc.
 *******************************************************************************/
#ifndef FADAS_H
#define FADAS_H

#include <stddef.h>
#include <stdint.h>

#if defined( __arm__ ) || defined( __aarch64__ ) || defined( __ARM_NEON__ )
#ifdef WIN32
#include "arm_neon.h"
typedef float float16_t;
#else
#include <arm_fp16.h>
#include <arm_neon.h>
#endif
#elif defined( __HEXAGON_ARCH__ )
typedef __fp16 float16_t;
typedef float float32_t;
typedef double float64_t;
#else
typedef float float16_t;
typedef float float32_t;
typedef double float64_t;
#endif

#ifdef __QNX__
#include <pmem.h>
typedef pmem_handle_t FadasMemHandle_t;
#else
typedef void *FadasMemHandle_t;
#endif

//==============================================================================
// Defines
//==============================================================================

#ifdef __GNUC__
#ifdef BUILDING_SO
// MACRO enables function to be visible in shared-library case.
#define FADAS_API __attribute__( ( visibility( "default" ) ) )
#define FADAS_LOCAL __attribute__( ( visibility( "hidden" ) ) )
#else
// MACRO empty for non-shared-library case.
#define FADAS_API
#endif
#else
#ifdef WIN32
// MACRO enables function to be visible in shared-library case.
#define FADAS_API __declspec( dllexport )
#else
// MACRO empty for non-shared-library case.
#define FADAS_API
#endif
#endif

#ifdef __GNUC__
#define FADAS_ALIGN32( VAR ) VAR __attribute__( ( aligned( 4 ) ) )
#define FADAS_ALIGN64( VAR ) VAR __attribute__( ( aligned( 8 ) ) )
#define FADAS_ALIGN128( VAR ) VAR __attribute__( ( aligned( 16 ) ) )
#define FADAS_ALIGN256( VAR ) VAR __attribute__( ( aligned( 32 ) ) )
#define FADAS_ALIGN1024( VAR ) VAR __attribute__( ( aligned( 128 ) ) )
#else
#define FADAS_ALIGN32( VAR ) __declspec( align( 4 ) )( VAR )
#define FADAS_ALIGN64( VAR ) __declspec( align( 8 ) )( VAR )
#define FADAS_ALIGN128( VAR ) __declspec( align( 16 ) )( VAR )
#define FADAS_ALIGN256( VAR ) __declspec( align( 32 ) )( VAR )
#define FADAS_ALIGN1024( VAR ) __declspec( align( 128 ) )( VAR )
#endif

#define FADAS_MODULO( x, y ) ( ( ( x ) + ( (y) -1 ) ) & ( ~( (y) -1 ) ) )
#define FADAS_MODULO128( x ) ( ( ( ( x ) + 127 ) >> 7 ) << 7 )

//==============================================================================
// Declarations
//==============================================================================

#ifdef __cplusplus
extern "C"
{
#endif

#define FADAS_NUM_IMAGE_PLANES ( 4 )

    /************************************************************************/ /**
     @brief
         Errors
     ****************************************************************************/
    typedef enum
    {
        FADAS_ERROR_NONE = 0,          ///<  No error.
        FADAS_ERROR_BAD_ARGUMENTS,     ///<  Bad arguments.
        FADAS_ERROR_BAD_OUTPUT,        ///<  Bad output.
        FADAS_ERROR_NULL_PTR,          ///<  NULL pointer.
        FADAS_ERROR_UNKNOWN,           ///<  Unknown error.
        FADAS_ERROR_FAIL,              ///<  Failure.
        FADAS_ERROR_NORES,             ///<  No resource of insufficient resource.
        FADAS_ERROR_INVALID_BUF,       ///<  Invalid buffer.
        FADAS_ERROR_UNSUPPORTED,       ///<  Unsupported.
        FADAS_ERROR_MAX = 0x7FFFFFFF   ///<  Do not use.
    } FadasError_e;


    /************************************************************************/ /**
     @brief
         Image format.

     @param FADAS_IMAGE_FORMAT_UNKNOWN
         Format is unspecified.

     @param FADAS_IMAGE_FORMAT_Y
         One-channel, 8-bit pixels.

     @param FADAS_IMAGE_FORMAT_Y12
         One-channel, 12-bit pixels in 16-bit containers.

     @param FADAS_IMAGE_FORMAT_UYVY
         8-bit UYUY.

     @param FADAS_IMAGE_FORMAT_UYVY10
         10-bit UYVY

     @param FADAS_IMAGE_FORMAT_VYUY
         8-bit VYUY

     @param FADAS_IMAGE_FORMAT_YUV888
         Three-channel, 24-bit pixels.

     @param FADAS_IMAGE_FORMAT_RGB888
         Three-channel, 24-bit pixels.

     @param FADAS_IMAGE_FORMAT_Y10UV10
         10-bit YUV420 semi-planar data in 16-bit container with LSB alignment.

     @param FADAS_IMAGE_FORMAT_Y8UV8
         8 bit luma, 8 bit chroma YUV420 semi-planar

     @param FADAS_IMAGE_FORMAT_BINARY
         Binary image with one byte per pixel.

     @param FADAS_IMAGE_FORMAT_COUNT
         Last color format.

     @param FADAS_IMAGE_FORMAT_MAX
         Do not use.
     ****************************************************************************/
    typedef enum
    {
        FADAS_IMAGE_FORMAT_UNKNOWN = 0,       ///<  Not specified
        FADAS_IMAGE_FORMAT_Y,                 ///<  1-channel, 8-bit pixels
        FADAS_IMAGE_FORMAT_Y12,               ///<  1-channel, 12-bit pixels in 16b containers
        FADAS_IMAGE_FORMAT_UYVY,              ///<  8-bit UYUY
        FADAS_IMAGE_FORMAT_UYVY10,            ///<  10-bit UYUY
        FADAS_IMAGE_FORMAT_VYUY,              ///<  8-bit VYUY
        FADAS_IMAGE_FORMAT_YUV888,            ///<  3-channel, 24-bit pixels
        FADAS_IMAGE_FORMAT_RGB888,            ///<  3-channel, 24-bit pixels
        FADAS_IMAGE_FORMAT_Y10UV10,           ///<  10 bit YUV420 semi-planar
                                              ///<  10 bits in 16-bit container
        FADAS_IMAGE_FORMAT_Y8UV8,             ///<  8 bit luma, 8 bit chroma
                                              ///<  YUV420 semi-planar
        FADAS_IMAGE_FORMAT_BINARY,            ///<  Binary image with 1-byte/pixel
        FADAS_IMAGE_FORMAT_COUNT,             ///<  Last color format
        FADAS_IMAGE_FORMAT_MAX = 0x7FFFFFFF   ///<  Do not use
    } FadasImageFormat_e;


    /************************************************************************/ /**
     @brief
         Lens distortion correction model.

     @param FADAS_LDC_NONE
         Pinhole camera with no lens distortion.

     @param FADAS_LDC_POLY2
         Two-parameter polynomial.

     @param FADAS_LDC_POLY3
         Three-parameter polynomial.

     @param FADAS_LDC_POLY4
         Four-parameter polynomial [k1, k2, p1, p2] plumb-line (a.k.a., Brown-
         Conrady) model [D. C. Brown, "Photometric Engineering", Vol. 32, No. 3,
         pp.444-462 (1966)].  Compatible with the oldest Caltech Matlab
         Calibration Toolbox (http://www.vision.caltech.edu/bouguetj/calib_doc/).
         To fill from OpenCV, declare cv::Mat for distortions with 5 rows (1
         columns), set it to zeros and use flag cv::CALIB_FIX_K3 with
         cv::calibrateCamera.\n

     @param FADAS_LDC_POLY5
         Five-parameter polynomial [k1, k2, p1, p2, k3] plumb-line model.
         Compatible with current Matlab toolbox.  To fill from OpenCV, declare
         cv::Mat for distortions with 5 rows, use flag cv::CALIB_FIX_K4 use
         cv::calibrateCamera.\n

     @param FADAS_LDC_RATIONAL6
         Six-parameter rational polynomial (\i i.e., CV_CALIB_RATIONAL_MODEL)
         [k1, k2, k3, k4, k5, k6].\n

     @param FADAS_LDC_RATIONAL8
         Eight-parameter rational polynomial.

     @param FADAS_LDC_FISHYEYE4
         Fisheye model [S.Shah, "Intrinsic Parameter Calibration Procedure for a
         (High-Distortion) Fish-eye Lens Camera with Distortion Model and Accuracy
         Estimation"].  To fill from OpenCV, use cv::fisheye::calibrate.

     @param FADAS_LDC_MAX
         \b WARNING: do not use.
     ****************************************************************************/
    typedef enum
    {
        FADAS_LDC_NONE = 0,          ///< Pinhole
        FADAS_LDC_POLY2,             ///< 2-parameter polynomial
        FADAS_LDC_POLY3,             ///< 3-parameter polynomial
        FADAS_LDC_POLY4,             ///< 4-parameter polynomial
        FADAS_LDC_POLY5,             ///< 5-parameter polynomial
        FADAS_LDC_RATIONAL6,         ///< 6-parameter rational
        FADAS_LDC_RATIONAL8,         ///< 8-parameter rational
        FADAS_LDC_FISHYEYE4,         ///< 4-parameter fisheye
        FADAS_LDC_END,               ///< To check invalid enum values
        FADAS_LDC_MAX = 0x7FFFFFFF   ///< \b WARNING: must be last
    } FadasLensDistortionModel_e;


    /************************************************************************/ /**
     @brief
         Image format conversion.
     ****************************************************************************/
    typedef enum
    {
        FADAS_IMAGE_CONVERSION_1C8_TO_1C8 = 0,     ///< One 8-bit channel unconverted
        FADAS_IMAGE_CONVERSION_3C888_TO_3C888,     ///< Three 8-bit channels unconverted
        FADAS_IMAGE_CONVERSION_UYVY_TO_RGB888,     ///< UYVY to RGB using parameters of
                                                   ///< FadasCvtYUV_UYVYtoRGB()
        FADAS_IMAGE_CONVERSION_YUV888_TO_RGB888,   ///< Three 8-bit channels YUV to RGB using
                                                   ///< parameters of FadasCvtYUV_UYVYtoRGB()
        FADAS_IMAGE_CONVERSION_MAX = 0x7FFFFFFF    ///< Do not use
    } FadasImageConversion_e;

    /************************************************************************/ /**
     @brief
         Buffer type for registration and deregistration.
     ****************************************************************************/
    typedef enum
    {
        FADAS_BUF_TYPE_NONE = 0,          ///< Buffer type is not available
        FADAS_BUF_TYPE_IN,                ///< Buffer is used only as input
        FADAS_BUF_TYPE_OUT,               ///< Buffer is used only as output
        FADAS_BUF_TYPE_INOUT,             ///< Buffer is used as both input and output
        FADAS_BUF_TYPE_END,               ///< Check invalid enum values
        FADAS_BUF_TYPE_MAX = 0x7fffffff   ///< Do not use
    } FadasBufType_e;

    /************************************************************************/ /**
     @brief
         Image properties structure.

     @param width
         Width of the image in pixels.

     @param height
         Height of the image in pixels.

     @param format
         Binary format of image pixel data.

     @param stride
         Memory width in bytes to the same pixel one row below.

     @param numPlanes
         Number of Image Planes
     ****************************************************************************/
    typedef struct
    {
        uint32_t width;
        uint32_t height;
        FadasImageFormat_e format;
        uint32_t stride[FADAS_NUM_IMAGE_PLANES];
        uint32_t numPlanes;
    } FadasImgProps_t;


    /************************************************************************/ /**
     @brief
         Image structure.

     @param plane
         Array of pointers containing the address to each image plane.

     @param props
         Image properties.

     @param bAllocated
         True if allocated, false if shared
     ****************************************************************************/
    typedef struct
    {
        void *plane[FADAS_NUM_IMAGE_PLANES];
        FadasImgProps_t props;
        bool bAllocated;
    } FadasImage_t;


    /************************************************************************/ /**
     @brief
         Camera calibration structure.

     @param focalLengthX
         Camera focal length in the X or U direction.

     @param focalLengthY
         Camera focal length in the Y or V direction.

     @param principalPointX
         Camera principal point in the X or U direction.

     @param principalPointY
         Camera principal point in the Y or V direction.
     ****************************************************************************/
    typedef struct
    {
        float64_t focalLengthX;
        float64_t focalLengthY;
        float64_t principalPointX;
        float64_t principalPointY;
    } FadasCameraProps_t;


    /************************************************************************/ /**
     @brief
         Image ROI structure.

     @param x
         X coordinate of upper-left corner of region of interest.
         \n\b WARNING: (x,y) pair must point to 128-byte aligned region.

     @param y
         Y coordinate of upper-left corner of region of interest.
         \n\b WARNING: (x,y) pair must point to 128-byte aligned region.

     @param width
         Width (pixels) of region of interest.  This must also equal the width of
         the destination region.
         \n NOTE: If set to 0, the width of the source image/map used.

     @param height
         Height (pixels) of region of interest.  This must also equal the height
         of the destination region.
         \n NOTE: If set to 0, the height of the source image/map used.
     ****************************************************************************/
    typedef struct
    {
        uint32_t x;
        uint32_t y;
        uint32_t width;
        uint32_t height;
    } FadasROI_t;


    /************************************************************************/ /**
     @brief
         Matrix properties structure.

     @param rows
         Number of rows in the matrix.

     @param cols
         Number of columns in the matrix

     @param stride
         Memory width in bytes for each row
     ****************************************************************************/
    typedef struct
    {
        uint32_t rows;
        uint32_t cols;
        uint32_t stride;
    } FadasMatProps_t;


    /************************************************************************/ /**
     @brief
         Renormalization parameters.

     @param sub
         Input constant (byte) to subtract from each pixel.

     @param mul
         Input constant to multiply each pixel remainder after the subtractions.

     @param add
         Input constant to add to each pixel after the multiplications.

     ****************************************************************************/
    typedef struct
    {
        float32_t sub;
        float32_t mul;
        float32_t add;
    } FadasNormlzParams_t;

    /************************************************************************/ /**
     @brief
         Structure to represent 32-bit signed 2D points

     @param x
         x value

     @param y
         y value

     ****************************************************************************/
    typedef struct
    {
        int32_t x;
        int32_t y;
    } FadasPt_2Di32_t;

    /************************************************************************/ /**
     @brief
         Structure to represent 32-bit unsigned 2D points

     @param x
         x value

     @param y
         y value

     ****************************************************************************/
    typedef struct FadasPt_2Du32
    {
        uint32_t x;
        uint32_t y;
    } FadasPt_2Du32_t;

    /************************************************************************/ /**
     @brief
         Structure to represent 32-bit floating point 2D points

     @param x
         x value

     @param y
         y value

     ****************************************************************************/
    typedef struct FadasPt_2Df32
    {
        float32_t x;
        float32_t y;
    } FadasPt_2Df32_t;


    /************************************************************************/ /**
     @brief
         Structure to represent 32-bit floating-point 3D points.

     @param x
         x co-ordinate.

     @param y
         y co-ordinate.

     @param z
         z co-ordinate.

     ****************************************************************************/
    typedef struct
    {
        float32_t x;
        float32_t y;
        float32_t z;
    } FadasPt_3Df32_t;

    /************************************************************************/ /**
     @brief
         Structure to represent sub-pixel accurate 3D box.

     @param x
         Sub-pixel accurate x co-ordinate of centre point.

     @param y
         Sub-pixel accurate y co-ordinate of centre point.

     @param z
         Sub-pixel accurate z co-ordinate of centre point.

     @param length
         Sub-pixel accurate depth of the 3D box.

     @param width
         Sub-pixel accurate width of the 3D box.

     @param height
         Sub-pixel accurate height of the 3D box.

     @param theta
         Orientation of the 3D box relative to the viewing angle.

     ****************************************************************************/
    typedef struct
    {
        float32_t x;
        float32_t y;
        float32_t z;
        float32_t length;
        float32_t width;
        float32_t height;
        float32_t theta;
    } FadasCuboidf32_t;

    /************************************************************************/ /**
     @brief
         Structure to describe a rectangular region.

     @param x
         x co-ordinate of upper-left corner of the rectangular region.

     @param y
         y co-ordinate of upper-left corner of the rectangular region.

     @param width
         Width of the rectangular region.

     @param height
         Height of the rectangular region.
     ****************************************************************************/
    typedef struct
    {
        float32_t x;
        float32_t y;
        float32_t width;
        float32_t height;
    } FadasRectf32_t;

    /************************************************************************/ /**
     @brief
         Structure to describe a 2D Grid.

     @param tl
         Coordinates of top left corner of the grid.

     @param br
         Coordinates of bottom right corner of the grid.

     @param cellSizeX
         Dimension of each cell along x-axis.

     @param cellSizeY
         Dimension of each cell along y-axis.
     ****************************************************************************/
    typedef struct
    {
        FadasPt_2Df32_t tl;
        FadasPt_2Df32_t br;
        float32_t cellSizeX;
        float32_t cellSizeY;
    } Fadas2DGrid_t;

    /************************************************************************/ /**
     @brief
         Return string of version information.

     @ingroup misc
     ****************************************************************************/
    FADAS_API
    const char *FadasVersion( void );


    /************************************************************************/ /**
     @brief
         Initialize FastADAS.
         \n\b WARNING: Must be called once before other FastADAS functions
         except FadasVersion().

     @param licenseKey
         Pointer to the license key string.

     @return
         #FADAS_ERROR_NONE -- Success.

     @ingroup misc
     ****************************************************************************/
    FADAS_API
    FadasError_e FadasInit( const char *licenseKey );


    /************************************************************************/ /**
     @brief
         Deinitialize FastADAS.
         \n\b WARNING: Must call this function once after all other FastADAS functions.

     @return
         #FADAS_ERROR_NONE -- Success.

     @ingroup misc
     ****************************************************************************/
    FADAS_API
    FadasError_e FadasDeInit( void );


    /************************************************************************/ /**
     @brief
         Allocate aligned memory.

     @param size
         Memory size in bytes.

     @param byteAlignment
         Memory alignment requirement in bytes.
         \n NOTE: Defaults to 64 byte alignment
         \n\b WARNING: Must be less than 128

     @param *handle
         Pointer to memory handle if using special memory region otherwise
         nullptr to use heap.

     @ingroup misc
     ****************************************************************************/
    FADAS_API
    void *FadasMemAlloc( size_t size, size_t byteAlignment, FadasMemHandle_t *handle = nullptr );


    /************************************************************************/ /**
     @brief
         Free memory allocated by FadasMalloc().

     @return
         #FADAS_ERROR_NONE -- Success.

     @ingroup misc
     ****************************************************************************/
    FADAS_API
    FadasError_e FadasMemFree( void *ptr );


    /************************************************************************/ /**
     @brief
         Allocate and set up the image structure using the width, height, and
         format.

     @param bufType
         Buffer type.

     @param imgW
         Image width (pixels).

     @param imgH
         Image height (pixels).

     @param imgFmt
         Image binary format and layout.

     @param img
         Image structure to allocate memory into.

     @param byteAlignment
         Memory alignment requirement in bytes.
         \n\b WARNING: Supported values are 16, 32, 64, and 128.

     @param *handle
         Pointer to memory handle if using special memory region otherwise
         nullptr to use heap.

     @ingroup misc
     ****************************************************************************/
    FADAS_API
    FadasError_e FadasCreateImage( FadasBufType_e bufType, uint32_t imgW, uint32_t imgH,
                                   FadasImageFormat_e imgFmt, FadasImage_t *img,
                                   size_t byteAlignment = 128, FadasMemHandle_t *handle = nullptr );


    /************************************************************************/ /**
     @brief
         Free memory that FadasCreateImage() allocated.

     @param img
         Pointer to the image structure from which to free memory.

     @return
         #FADAS_ERROR_NONE -- Success.

     @ingroup misc
     ****************************************************************************/
    FADAS_API
    FadasError_e FadasDestroyImage( FadasImage_t *img );

    /************************************************************************/ /**
     @brief
         Copy image contents with same dimensions.

     @param src
         Source image structure to be copied from.

     @param dst
         Destination image structure to be copied to.

     @return
         FADAS_ERROR_NONE if successful.

     @ingroup misc
     ****************************************************************************/
    FADAS_API
    FadasError_e FadasCopyImage( const FadasImage_t &src, FadasImage_t &dst );


    /************************************************************************/ /**
     @brief
         Register buffer with FastADAS.
         \n NOTE: Only use buffers registered with FastADAS for processing.
         \n NOTE: Only register I/O buffers at initialization.

     @param bufType
         Type of buffer to be registered.

     @param buf
         Pointer to the buffer to register.

     @param bufSize
         Buffer size.

     @ingroup misc
     ****************************************************************************/
    FADAS_API
    FadasError_e FadasRegBuf( FadasBufType_e bufType, const void *buf, size_t bufSize );

    /************************************************************************/ /**
     @brief
         Deregister buffer with FastADAS.
         \nNOTE: Do not use buffers for processing after they deregister with FastADAS.

     @param buf
         Buffer to deregister.

     @ingroup misc
     ****************************************************************************/
    FADAS_API
    FadasError_e FadasDeregBuf( const void *buf );

#ifdef __cplusplus
}
#endif


#include <fadasRemap.h>
#include <fadasVM.h>
#endif /* FADAS_H */
