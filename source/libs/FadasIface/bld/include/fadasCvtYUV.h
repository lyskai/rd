/***************************************************************************//**
@file
    fadasCvtYUV.h

@brief
    Image Conversion

@defgroup cvtyuv Image Conversion

@defgroup cvtyuvh Image Conversion Helpers

@details
    Image conversion and scaling module.

@internal
    Copyright 2020-2022 Qualcomm Technologies, Inc.  All Rights Reserved.
    Confidential and Proprietary - Qualcomm Technologies, Inc.
*******************************************************************************/
#ifndef FADASCVTYUV_H
#define FADASCVTYUV_H

#include <stdint.h>



#ifdef __cplusplus
extern "C"
{
#endif



/************************************************************************//**
@brief
    Algorithm pipeline.

@param FADAS_CVTYUV_PIPELINE_DownscaleUYVYAndRGB888
    Scale UYUV image down by 1x to 8x in each dimension and convert to RGB888.
    The ratios in height and width need not be the same.

@param FADAS_CVTYUV_PIPELINE_DownscaleUYVYBy2AndRGB888
    Scale UYUV image down by two in each dimension and convert to RGB888.

@param FADAS_CVTYUV_PIPELINE_DownscaleYUV888AndRGB888
    Scale YUV888 image down by 1x to 8x in each dimension and convert to RGB888.
    The ratios in height and width need not be the same.

@param FADAS_CVTYUV_PIPELINE_DownscaleYUV888By2AndRGB888
    Scale YUV888 image down by two in each dimension and convert to RGB888.

@param FADAS_CVTYUV_PIPELINE_DownscaleY8UV8By2
    Scale Y8UV8 image down by two in each dimension.

@param FADAS_CVTYUV_PIPELINE_DownscaleY10UV10By2
    Scale Y10UV10 image down by two in each dimension.

@param FADAS_CVTYUV_PIPELINE_DownscaleY8UV8
    Scale Y8UV8 image down by 1x to 8x in each dimension and the ratios in
    height and width need not be the same.

@param FADAS_CVTYUV_PIPELINE_DownscaleY8UV8By2
    Scale Y8UV8 image down by two in each dimension and the ratios in height
    and width need not be the same.

@param FADAS_CVTYUV_PIPELINE_DownscaleVYUYAndRGB888
    Scale VYUY image down by 1x to 8x in each dimension and convert to RGB888.
    The ratios in height and width need not be the same.

@param FADAS_CVTYUV_PIPELINE_DownscaleVYUYBy2AndRGB888
    Scale VYUY image down by two in each dimension and convert to RGB888.

@param FADAS_CVTYUV_PIPELINE_Downscale3C888
    Scale three-channel 8-bit image down by 1x to 8x in each dimension.
    The ratios in height and width need not be the same

@param FADAS_CVTYUV_PIPELINE_UYVYtoRGBAndNormi8
    Convert UYUV image to RGB888 and normalize the pixels.

@param FADAS_CVTYUV_PIPELINE_DownscaleUYVYBy2ToRGB888AndNormi8
    Scale UYUV image down by two in each dimension, convert to RGB888 and normalize the pixels.

@ingroup cvtyuv
****************************************************************************/
typedef enum
{
    FADAS_CVTYUV_PIPELINE_DownscaleUYVYAndRGB888 = 0,       ///< UYVY to RGB using the parameters of FadasCvtYUV_UYVYtoRGB()
    FADAS_CVTYUV_PIPELINE_DownscaleUYVYBy2AndRGB888,        ///< UYVY to RGB using parameters of FadasCvtYUV_UYVYtoRGB()
    FADAS_CVTYUV_PIPELINE_DownscaleYUV888AndRGB888,         ///< 3 8-bit channel YUV to RGB using parameters of FadasCvtYUV_UYVYtoRGB()
    FADAS_CVTYUV_PIPELINE_DownscaleYUV888By2AndRGB888,      ///< 3 8-bit channel YUV to RGB using parameters of FadasCvtYUV_UYVYtoRGB()
    FADAS_CVTYUV_PIPELINE_DownscaleY8UV8By2,                ///< Multi planar image (Y,UV) with 8 bit data in 8 bit carriers
    FADAS_CVTYUV_PIPELINE_DownscaleY10UV10By2,              ///< Multi planar image (Y,UV) with 10 bit data in 16 bit carriers
    FADAS_CVTYUV_PIPELINE_DownscaleVYUYAndRGB888,           ///< VYUY to RGB using parameters of FadasCvtYUV_UYVYtoRGB()
    FADAS_CVTYUV_PIPELINE_DownscaleVYUYBy2AndRGB888,        ///< VYUY to RGB using parameters of FadasCvtYUV_UYVYtoRGB()
    FADAS_CVTYUV_PIPELINE_Downscale3C888,                   ///< 3 8-bit channel (YUV or RGB)
    FADAS_CVTYUV_PIPELINE_UYVYtoRGBAndNormi8,               ///< UYVY to RGB using parameters of FadasCvtYUV_UYVYtoRGB() and Normalization using parameters of FadasCvtYUV_Renormalize888u8i8()
    FADAS_CVTYUV_PIPELINE_DownscaleUYVYBy2ToRGB888AndNormi8,///< UYVY By 2 to RGB using parameters of FadasCvtYUV_UYVYtoRGB() and Normalization using parameters of FadasCvtYUV_Renormalize888u8i8()
    FADAS_CVTYUV_PIPELINE_END,                              ///< To check invalid enum values
    FADAS_CVTYUV_PIPELINE_MAX = 0x7FFFFFFF                  ///< \b WARNING: do not use
} FadasCvtYUVPipeline_e;

/************************************************************************//**
@brief
    Initialize color conversion module.
    \n\b WARNING: Must be called once before other FastADAS functions
    except FadasVersion().

@param licenseKey
    License key string.

 @return
    #FADAS_ERROR_NONE -- Success.

@ingroup cvtyuv
****************************************************************************/
FADAS_API
FadasError_e FadasCvtYUV_Init( const char *licenseKey );



/************************************************************************//**
@brief
    De-initialize color conversion module.
    \n\b WARNING: Must be called once after all other FastADAS functions.

@return
    #FADAS_ERROR_NONE -- Success.

@ingroup cvtyuv
****************************************************************************/
FADAS_API
FadasError_e FadasCvtYUV_DeInit( void );



/************************************************************************//**
@brief
    Creates a multithreaded worker pool for the YUV conversion
    feature.

@param nThreads
    Requested number of threads; can be limited to the maximum number
    of threads if requesting more than that limit. The maximum is the
    maximum number of physical or logical cores. If requesting 0 threads,
    use the maximum number

@param pThreadsAffinity
    Array to set threads affinity

@param eType
    Descriptor of execution pipeline to use.

@return
    Worker pool pointer.

@ingroup cvtyuv
****************************************************************************/
FADAS_API
void* FadasCvtYUV_CreateWorkers( uint32_t              nThreads,
                                 int32_t               pThreadsAffinity[],
                                 FadasCvtYUVPipeline_e eType );



/************************************************************************//**
@brief
    Destroys YUV conversion worker pool.

@param wrkrs
    Worker pool created by FadasCvtYUV_CreateWorkers().

@return
    #FADAS_ERROR_NONE -- Success.

@ingroup cvtyuv
****************************************************************************/
FADAS_API
FadasError_e FadasCvtYUV_DestroyWorkers( void* wrkrs );



/************************************************************************//**
@brief
    Runs a multithreaded pipeline for this CvtYUV module.

@details
    Runs a multithreaded pipeline using a worker pool created by
    FadasCvtYUV_CreateWorkers().  At worker pool creation, the
    pipeline of one or more chained algorithms is defined.

@param wrkrs
    Worker pool created by FadasCvtYUV_CreateWorkers().

@param src
    Pointer to the input image. The size of buffer is srcStride*srcHeight bytes.
    \n\b WARNING: Must be 128-byte aligned.

@param dst
    Pointer to the output image with the same type and size as the input image. The
    size of buffer is dstStride*dstHeight bytes.
    \n\b WARNING: Must be 128-byte aligned.

@param dstROI
    Region of interest in output image.

@param normlz
    Pointer to array of structure holding normalization parameters for all channel.
    Optional parameter, required only when output image is to be normalized.
    \n\b WARNING:
    \li normlz[i].sub \f$(s_r)\f$ has a limited valid range of \f$-255 \le s_r \le 255\f$.
    \li normlz[i].mul \f$(m_r)\f$ has a limited valid range of \f$-4 \le m_r \le 4\f$.
    \li normlz[i].add \f$(a_r)\f$ has a limited valid range of \f$-255 \le a_r \le 255\f$.

@return
    #FADAS_ERROR_NONE -- Success.

@ingroup cvtyuv
****************************************************************************/
FADAS_API
FadasError_e FadasCvtYUV_RunMT( void                *wrkrs,
                                FadasImage_t        *src,
                                FadasImage_t        *dst,
                                FadasROI_t          *dstROI,
                                FadasNormlzParams_t *normlz = nullptr);

/************************************************************************//**
@brief
    Converts an 8-bit UYVY image to RGB888 using parameters of
    FadasCvtYUV_UYVYtoRGB() and downscales by both sqrt(2) and 2
    in dimensions.  Both downscaling operations must be done and are not
    optional.  If only one downscaling is needed then use separate functions.

    \n\b WARNING: Obsolete.

@param src
    Input 8-bit UYVY source image.
    \n\b WARNING: Must be 128-byte aligned.

@param srcProps
    Input image properties.
    \n\b WARNING: Stride (bytes) must be a multiple of 128.

@param dst
    Output RGB888 destination image.
    \n\b WARNING: Adequate memory must already be allocated to the image buffer.

@param dstStride
    Stride (bytes) for dst [dst_stride >= dst_w * 3].  If set to 0 before
    calling, an appropriate value will be assigned.
    \n\b WARNING: Must be a multiple of 128.

@param dstS2
    Output RGB888 image downscaled by sqrt(2).
    \n\b WARNING:
    \li Adequate memory must already be allocated to the image buffer.
    \li Must be 128-byte aligned.

@param dstS2Props
    Downscaled image (dstS2) properties
    \n\b WARNING: Must be a multiple of 128.

@param dst2
    Pointer to the output image.
    \n\b WARNING:
    \li Adequate memory must already be allocated to the image buffer.
    \li Must be 128-byte aligned.

@param dst2Props
    Downscaled image (dst2) properties
    \n\b WARNING: Must be a multiple of 128.

@return
    #FADAS_ERROR_NONE -- Success.

@ingroup cvtyuv
****************************************************************************/
FADAS_API
FadasError_e FadasCvtYUV_UYVYtoRGB888andScale( const uint8_t   *__restrict src,
                                               FadasImgProps_t srcProps,
                                               uint8_t         *__restrict dst,
                                               uint32_t        dstStride,
                                               uint8_t         *__restrict dstS2,
                                               FadasImgProps_t *dstS2Props,
                                               uint8_t         *__restrict dst2,
                                               FadasImgProps_t *dst2Props );

/************************************************************************//**
@brief
    Converts an 8-bit UYVY image to grayscale.

@param src
    Pointer to the input image.
    \n\b WARNING: Must be 128-byte aligned.

@param srcProps
    Input image properties.
    \n\b WARNING: Must be a multiple of 128.

@param dst
    Pointer to the output grayscale image.
    \n\b WARNING: Must be 128-byte aligned.

@param dstStride
    Stride (bytes) for dst.
    \n\b WARNING: Must be a multiple of 128.

@return
    #FADAS_ERROR_NONE -- Success.

@ingroup cvtyuvh
****************************************************************************/
FADAS_API
FadasError_e FadasCvtYUV_UYVYtoGray( const uint8_t   *__restrict src,
                                     FadasImgProps_t srcProps,
                                     uint8_t         *__restrict dst,
                                     uint32_t        dstStride );

/************************************************************************//**
@brief
    Converts an 8-bit VYUY image to grayscale.

@param src
    Pointer to the input image.
    \n\b WARNING: Must be 128-byte aligned.

@param srcProps
    Input image properties.
    \n\b WARNING: Must be a multiple of 128.

@param dst
    Pointer to the output grayscale image.
    \n\b WARNING: Must be 128-byte aligned.

@param dstStride
    Stride (bytes) for dst.
    \n\b WARNING: Must be a multiple of 128.

@return
    #FADAS_ERROR_NONE -- Success.

@ingroup cvtyuvh
****************************************************************************/
FADAS_API
FadasError_e FadasCvtYUV_VYUYtoGray( const uint8_t   *__restrict src,
                                     FadasImgProps_t srcProps,
                                     uint8_t         *__restrict dst,
                                     uint32_t        dstStride );

/************************************************************************//**
@brief
    Converts an 8-bit UYVY image to NV12.

@param src
    Pointer to the input image.
    \n\b WARNING: Must be 128-byte aligned.

@param srcProps
    Input image properties.
    \n\b WARNING: Must be a multiple of 128.

@param dstY
    Pointer to the luma (Y) component of the output image.
    \n\b WARNING: Must be 128-byte aligned.

@param dstYStride
    Stride (bytes) for the dstY parameter.
    \n\b WARNING: Must be a multiple of 128.

@param dstUV
    Pointer to the UY pseudo planes of the output image.
    \n\b WARNING: Must be 128-byte aligned.

@param dstUVStride
    Stride (bytes) for the dstUV parameter.
    \n\b WARNING: Must be a multiple of 128.

@return
    #FADAS_ERROR_NONE -- Success.

@ingroup cvtyuvh
****************************************************************************/
FADAS_API
FadasError_e FadasCvtYUV_UYVYtoNV12( const uint8_t   *__restrict src,
                                     FadasImgProps_t srcProps,
                                     uint8_t         *__restrict dstY,
                                     uint32_t        dstYStride,
                                     uint8_t         *__restrict dstUV,
                                     uint32_t        dstUVStride );


/************************************************************************//**
@brief
    Converts an 8-bit VYUY image to NV12.

@param src
    Pointer to the input image.
    \n\b WARNING: Must be 128-byte aligned.

@param srcProps
    Input image properties.
    \n\b WARNING: Must be a multiple of 128.

@param dstY
    Pointer to the luma (Y) component of the output image.
    \n\b WARNING: Must be 128-byte aligned.

@param dstYStride
    Stride (bytes) for the dstY parameter.
    \n\b WARNING: Must be a multiple of 128.

@param dstUV
    Pointer to the UY pseudo planes of the output image.
    \n\b WARNING: Must be 128-byte aligned.

@param dstUVStride
    Stride (bytes) for the dstUV parameter.
    \n\b WARNING: Must be a multiple of 128.

@return
    #FADAS_ERROR_NONE -- Success.

@ingroup cvtyuvh
****************************************************************************/
FADAS_API
FadasError_e FadasCvtYUV_VYUYtoNV12( const uint8_t   *__restrict src,
                                     FadasImgProps_t srcProps,
                                     uint8_t         *__restrict dstY,
                                     uint32_t        dstYStride,
                                     uint8_t         *__restrict dstUV,
                                     uint32_t        dstUVStride );


/************************************************************************//**
@brief
    Converts 10-bit UYVY image to NV12.

@param src
    Pointer to the input image.
    \n\b WARNING: Must be 128-byte aligned.

@param srcProps
    Input image properties.
    \n\b WARNING: Stride (bytes) must be a multiple of 128.

@param dstY
    Pointer to the luma (Y) component of the output image.
    \n\b WARNING: Must be 128-byte aligned.

@param dstYStride
    Stride (bytes) for the dstY parameter.
    \n\b WARNING: Must be a multiple of 128.

@param dstUV
    Pointer to the UY pseudo planes of the output image.
    \n\b WARNING: Must be 128-byte aligned.

@param dstUVStride
    Stride (bytes) for the dstUV parameter.
    \n\b WARNING: Must be a multiple of 128.

@return
    #FADAS_ERROR_NONE -- Success.

@ingroup cvtyuvh
****************************************************************************/
FADAS_API
FadasError_e FadasCvtYUV_UYVY10toNV12( const uint8_t   *__restrict src,
                                       FadasImgProps_t srcProps,
                                       uint8_t         *__restrict dstY,
                                       uint32_t        dstYStride,
                                       uint8_t         *__restrict dstUV,
                                       uint32_t        dstUVStride );



/************************************************************************//**
@brief
    Converts an 8-bit UYVY image to RGB888. Using the equations of <i>The Image
    Processing Handbook</i> (Russ and Neal 2015) and dropping the primes for
    convenience gives:

    \f[
    Y=K_R R + K_G G + K_B B
    \f]

    \f[
    C_b = 0.5 \left( \frac{B-Y}{1-K_B} \right)
    \f]

    \f[
    C_r = 0.5 \left( \frac{R-Y}{1-K_R} \right)
    \f]

    and inverts to:

    \f[
    \begin{bmatrix}R\\G\\B\end{bmatrix} =

    \begin{bmatrix}1 & 0 & 2(1-K_R)\\1 & -\frac{2( K_R (1-K_R)}{K_G} & -\frac{2( K_B (1-K_B)}{K_G}\\1 & 2(1-K_B) & 0\end{bmatrix}

    \begin{bmatrix}Y\\C_b\\C_r\end{bmatrix}
    \f]

    The color conversion parameters follow those from the ITU-R BT.601-7
    recommendation of \f$K_R=0.299\f$, \f$K_B=0.114\f$, and \f$K_G=0.587\f$.

    \f[
    \begin{bmatrix}R\\G\\B\end{bmatrix} =

    \begin{bmatrix}1 & 0 & 1.402000\\1 & -0.344136 & -0.714136\\1 & 1.772000 & 0\end{bmatrix}

    \begin{bmatrix}Y\\C_b\\C_r\end{bmatrix}
    \f]

    The OpenCV equivalent can be:
    \code
    FadasImgProps_t srcProps = { w, h, 2*w };
    uint32_t dstStride = 3*w;

    FadasCvtYUV_UYVYtoYUV_R( uyvyimg, srcProps, yuvimg, dstStride );

    for( int r=0; r<h; ++r )
       for( int c = 0; c < w; ++c )
       {
           int idx = 3 * (w * r + c);
           uint8_t u = yuvimg[idx + 1];
           uint8_t v = yuvimg[idx + 2];
           yuvimg[idx + 1] = v;
           yuvimg[idx + 2] = u;
       }

    cv::Mat ocvYUVImage( h, w, CV_8UC3, const_cast<uint8_t*>(yuvimg) );
    cv::Mat ocvRGBImage( h, w, CV_8UC3, const_cast<uint8_t*>(rgbimg) );
    cv::cvtColor( ocvYUVImage, ocvRGBImage, cv::COLOR_YCrCb2RGB );
    \endcode

@param src
    Pointer to the input image.
    \n\b WARNING: Must be 128-byte aligned.

@param srcProps
    Input image properties.
    \n\b WARNING: Stride (bytes) must be a multiple of 128.

@param dst
    Pointer to the output image.
    \n\b WARNING: Must be 128-byte aligned.

@param dstStride
    Stride (bytes) for dst [dstStride >= width * 3]
    \n\b WARNING: Must be a multiple of 128.

@return
    #FADAS_ERROR_NONE -- Success.

@ingroup cvtyuvh
****************************************************************************/
FADAS_API
FadasError_e FadasCvtYUV_UYVYtoRGB( const uint8_t   *__restrict src,
                                    FadasImgProps_t srcProps,
                                    uint8_t         *__restrict dst,
                                    uint32_t        dstStride );


/************************************************************************//**
@brief
    Same as FadasCvtYUV_UYVYtoRGB() with the additional step of normalization
    at the end

@param src
    Pointer to the input image.
    \n\b WARNING: Must be 128-byte aligned.

@param srcProps
    Input image properties.
    \n\b WARNING: Stride (bytes) must be a multiple of 128.

@param normlzR
    Image normalization parameters for red channel.
    \n\b WARNING:
    \li normlz.sub \f$(s_r)\f$ has a limited valid range of \f$-255 \le s_r \le 255\f$.
    \li normlz.mul \f$(m_r)\f$ has a limited valid range of \f$-4 \le m_r \le 4\f$.
    \li normlz.add \f$(a_r)\f$ has a limited valid range of \f$-255 \le a_r \le 255\f$.

@param normlzG
    Image normalization parameters for green channel.
    \n\b WARNING:
    \li normlz.sub \f$(s_g)\f$ has a limited valid range of \f$-255 \le s_g \le 255\f$.
    \li normlz.mul \f$(m_g)\f$ has a limited valid range of \f$-4 \le m_g \le 4\f$.
    \li normlz.add \f$(a_g)\f$ has a limited valid range of \f$-255 \le a_g \le 255\f$.

@param normlzB
    Image normalization parameters for blue channel.
    \n\b WARNING:
    \li normlz.sub \f$(s_b)\f$ has a limited valid range of \f$-255 \le s_b \le 255\f$.
    \li normlz.mul \f$(m_b)\f$ has a limited valid range of \f$-4 \le m_b \le 4\f$.
    \li normlz.add \f$(a_b)\f$ has a limited valid range of \f$-255 \le a_b \le 255\f$.

@param dst
    Pointer to the output image.
    \n\b WARNING: Must be 128-byte aligned.

@param dstStride
    Stride (bytes) for dst [dstStride >= width * 3]

@return
    #FADAS_ERROR_NONE -- Success.

@ingroup cvtyuvh
****************************************************************************/
FADAS_API
FadasError_e FadasCvtYUV_UYVYtoRGBAndNormi8( const uint8_t       *src,
                                             FadasImgProps_t     srcProps,
                                             FadasNormlzParams_t normlzR,
                                             FadasNormlzParams_t normlzG,
                                             FadasNormlzParams_t normlzB,
                                             int8_t              *dst,
                                             uint32_t            dstStride );


/************************************************************************//**
@brief
    Converts an 8-bit VYUY image to RGB888. See \ref FadasCvtYUV_UYVYtoRGB
    for more details

@param src
    Pointer to the input image.
    \n\b WARNING: Must be 128-byte aligned.

@param srcProps
    Input image properties.
    \n\b WARNING: Stride (bytes) must be a multiple of 128.

@param dst
    Pointer to the output image.
    \n\b WARNING: Must be 128-byte aligned.

@param dstStride
    Stride (bytes) for dst [dstStride >= width * 3]
    \n\b WARNING: Must be a multiple of 128.

@return
    #FADAS_ERROR_NONE -- Success.

@ingroup cvtyuvh
****************************************************************************/
FADAS_API
FadasError_e FadasCvtYUV_VYUYtoRGB( const uint8_t   *__restrict src,
                                    FadasImgProps_t srcProps,
                                    uint8_t         *__restrict dst,
                                    uint32_t        dstStride );


/************************************************************************//**
@brief
    Converts 10-bit UYVY image to RGB888.  The color conversion parameters of
    FadasCvtYUV_UYVYtoRGB() are used.

@param src
    Pointer to the input image.
    \n\b WARNING: Must be 128-byte aligned.

@param srcProps
    Input image properties.
    \n\b WARNING: Stride (bytes) must be a multiple of 128.

@param dst
    Pointer to the output image.
    \n\b WARNING: Must be 128-byte aligned.

@param dstStride
    Stride (bytes) for dst [dstStride >= width * 3]
    \n\b WARNING: Must be a multiple of 128.

@return
    #FADAS_ERROR_NONE -- Success.

@ingroup cvtyuvh
****************************************************************************/
FADAS_API
FadasError_e FadasCvtYUV_UYVY10toRGB( const uint8_t   *__restrict src,
                                      FadasImgProps_t srcProps,
                                      uint8_t         *__restrict dst,
                                      uint32_t        dstStride );



/************************************************************************//**
@brief
    Converts an 8-bit UYVY image to YUV888.

@param src
    Pointer to the input image.
    \n\b WARNING: Must be 128-byte aligned.

@param srcProps
    Input image properties.
    \n\b WARNING: Stride (bytes) must be a multiple of 128.

@param dst
    Pointer to the output image.
    \n\b WARNING: Must be 128-byte aligned.

@param dstStride
    Stride (bytes) for dst
    \n\b WARNING: Must be a multiple of 128.

@return
    #FADAS_ERROR_NONE -- Success.

@ingroup cvtyuvh
****************************************************************************/
FADAS_API
FadasError_e FadasCvtYUV_UYVYtoYUV( const uint8_t   *__restrict src,
                                    FadasImgProps_t srcProps,
                                    uint8_t         *__restrict dst,
                                    uint32_t        dstStride );

/************************************************************************//**
@brief
    Converts an 8-bit VYUY image to YUV888.

@param src
    Pointer to the input image.
    \n\b WARNING: Must be 128-byte aligned.

@param srcProps
    Input image properties.
    \n\b WARNING: Stride (bytes) must be a multiple of 128.

@param dst
    Pointer to the output image.
    \n\b WARNING: Must be 128-byte aligned.

@param dstStride
    Stride (bytes) for dst
    \n\b WARNING: Must be a multiple of 128.

@return
    #FADAS_ERROR_NONE -- Success.

@ingroup cvtyuvh
****************************************************************************/
FADAS_API
FadasError_e FadasCvtYUV_VYUYtoYUV( const uint8_t   *__restrict src,
                                    FadasImgProps_t srcProps,
                                    uint8_t         *__restrict dst,
                                    uint32_t        dstStride );



/************************************************************************//**
@brief
    Scale a single-channel image (for example, grayscale) down by two in dimension (four in
    resolution).

@param src
    Pointer to the input image.
    \n\b WARNING: Must be 128-byte aligned.

@param srcProps
    Input image properties.
    \n\b WARNING: Stride (bytes) must be a multiple of 128.

@param dst
    Pointer to the output image.
    \n\b WARNING:
    \li Adequate memory must already be allocated to the image buffer.
    \li Must be 128-byte aligned.

@param dstProps
    Output image properties.
    \n\b WARNING: Stride (bytes) must be a multiple of 128.

@return
    #FADAS_ERROR_NONE -- Success.

@ingroup cvtyuvh
****************************************************************************/
FADAS_API
FadasError_e FadasCvtYUV_DownscaleBy2( const uint8_t   *__restrict src,
                                       FadasImgProps_t srcProps,
                                       uint8_t         *__restrict dst,
                                       FadasImgProps_t *dstProps );



/************************************************************************//**
@brief
    Scale multiplane image (for example, Y, UV) down by two in dimension (four in
    resolution).

@param src
    Pointer to the input image object of type #FadasImage_t

@param dst
    Pointer to the output image object of type #FadasImage_t

@return
    #FADAS_ERROR_NONE -- Success.

@ingroup cvtyuv
****************************************************************************/
FADAS_API
FadasError_e FadasCvtYUV_DownscaleY8UV8by2( const FadasImage_t *__restrict src,
                                            FadasImage_t       *__restrict dst );



/************************************************************************//**
@brief
    Scale multiplane image (for example, Y, UV) down by two in dimension (four in
    resolution).

@param src
    Pointer to the input image object of type #FadasImage_t

@param dst
    Pointer to the output image object of type #FadasImage_t

@return
    #FADAS_ERROR_NONE -- Success.

@ingroup cvtyuv
****************************************************************************/
FADAS_API
FadasError_e FadasCvtYUV_DownscaleY10UV10by2( const FadasImage_t *__restrict src,
                                              FadasImage_t       *__restrict dst );


/************************************************************************//**
@brief
    Scale a RGB888 image down by two in dimension (four in resolution).

@param src
    Pointer to the input image.
    \n\b WARNING: Must be 128-byte aligned.

@param srcProps
    Input image properties.
    \n\b WARNING: Stride (bytes) must be a multiple of 128.

@param dst
    Pointer to the output image.
    \n\b WARNING:
    \li Adequate memory must already be allocated to the image buffer.
    \li Must be 128-byte aligned.

@param dstProps
    Output image properties.
    \n\b WARNING: Stride (bytes) must be a multiple of 128.

@return
    #FADAS_ERROR_NONE -- Success.

@ingroup cvtyuvh
****************************************************************************/
FADAS_API
FadasError_e FadasCvtYUV_Downscale888by2( const uint8_t   *__restrict src,
                                          FadasImgProps_t srcProps,
                                          uint8_t         *__restrict dst,
                                          FadasImgProps_t *dstProps );



/************************************************************************//**
@brief
    Scale image down by two in resolution or sqrt(2) in dimensions.

@param src
    Pointer to the input image.
    \n\b WARNING: Must be 128-byte aligned.

@param srcProps
    Input image properties.
    Stride (bytes) for src.
    \n\b WARNING: Must be a multiple of 128.

@param dst
    Pointer to the output image.
    \n\b WARNING:
    \li Adequate memory must already be allocated to the image buffer.
    \li Must be 128-byte aligned.

@param dstProps
    Output image properties
    \n\b WARNING: Stride (bytes) must be a multiple of 128.

@return
    #FADAS_ERROR_NONE -- Success.

@ingroup cvtyuvh
****************************************************************************/
FADAS_API
FadasError_e FadasCvtYUV_DownscaleBySqrt2( const uint8_t   *__restrict src,
                                           FadasImgProps_t srcProps,
                                           uint8_t         *__restrict dst,
                                           FadasImgProps_t *dstProps );



/************************************************************************//**
@brief
    Scale RGB888 image down by two in resolution or sqrt(2) in dimensions.

@param src
    Pointer to the input image.
    \n\b WARNING: Must be 128-byte aligned.

@param srcProps
    Input image properties.
    \n\b WARNING: Stride (bytes) must be a multiple of 128.

@param dst
    Pointer to the output image.
    \n\b WARNING:
    \li Adequate memory must already be allocated to the image buffer.
    \li Must be 128-byte aligned.

@param dstProps
    Output image properties.
    \n\b WARNING: Stride (bytes) must be a multiple of 128.

@return
    #FADAS_ERROR_NONE -- Success.

@ingroup cvtyuvh
****************************************************************************/
FADAS_API
FadasError_e FadasCvtYUV_Downscale888bySqrt2( const uint8_t   *__restrict src,
                                              FadasImgProps_t srcProps,
                                              uint8_t         *__restrict dst,
                                              FadasImgProps_t *dstProps );



/************************************************************************//**
@brief
    Scale image down by 1x to 2x in each dimension and the ratios in height and
    width need not be the same.

@param src
    Pointer to the input image.
    \n\b WARNING: Must be 128-byte aligned.

@param srcProps
    Input image properties.
    \n\b WARNING: Stride (bytes) must be a multiple of 128.

@param dst
    Pointer to the output image.
    \n\b WARNING:
    \li Adequate memory must already be allocated to the image buffer.
    \li Must be 128-byte aligned.

@param dstProps
    Output image properties.
    \n\b WARNING: Stride (bytes) must be a multiple of 128.

@return
    #FADAS_ERROR_NONE -- Success.

@ingroup cvtyuvh
****************************************************************************/
FADAS_API
FadasError_e FadasCvtYUV_Downscale( const uint8_t   *__restrict src,
                                    FadasImgProps_t srcProps,
                                    uint8_t         *__restrict dst,
                                    FadasImgProps_t dstProps );



/************************************************************************//**
@brief
    Scale RGB image down by 1x to 2x in each dimension and the ratios in height
    and width need not be the same.

@param src
    Pointer to the input image.
    \n\b WARNING: Must be 128-byte aligned.

@param srcProps
    Input image width (pixels).
    \n\b WARNING: Stride (bytes) must be a multiple of 128.

@param dst
    Pointer to the output image.
    \n\b WARNING:
    \li Adequate memory must already be allocated to the image buffer.
    \li Must be 128-byte aligned.

@param dstProps
    Output image properties.
    \n\b WARNING: Stride (bytes) must be a multiple of 128.

@return
    #FADAS_ERROR_NONE -- Success.

@ingroup cvtyuvh
****************************************************************************/
FADAS_API
FadasError_e FadasCvtYUV_Downscale888( const uint8_t   *__restrict src,
                                       FadasImgProps_t srcProps,
                                       uint8_t         *__restrict dst,
                                       FadasImgProps_t dstProps );

/************************************************************************//**
@brief
    Scale multiplane image (for example, Y, UV with 8 bit data) down by 1x to 8x in
    each dimension. The ratios in height and width need not be the same.

@param src
    Pointer to the input image object of type #FadasImage_t.

@param dst
    Pointer to the output image object of type #FadasImage_t.

@return
    #FADAS_ERROR_NONE -- Success.

@ingroup cvtyuv
****************************************************************************/
FADAS_API
FadasError_e FadasCvtYUV_DownscaleY8UV8( const FadasImage_t *src,
                                         FadasImage_t       *dst );

/************************************************************************//**
@brief
    Scale multiplane image (for example, Y, UV with 10 bit data in 16 bit carriers) down
    by 1x to 8x in each dimension. The ratios in height and width need not be the same.

@param src
    Pointer to the input image object of type #FadasImage_t.

@param dst
    Pointer to the output image object of type #FadasImage_t.

@return
    #FADAS_ERROR_NONE -- Success.

@ingroup cvtyuv
****************************************************************************/

FADAS_API
FadasError_e FadasCvtYUV_DownscaleY10UV10( const FadasImage_t *src,
                                           FadasImage_t       *dst );



/************************************************************************//**
@brief
    Scale YUV888 image down by 1x to 8x in each dimension, the ratios in
    height and width need not be the same, and convert to RGB888.

@param src
    Pointer to the input image.
    \n\b WARNING: Must be 128-byte aligned.

@param srcProps
    Input image width (pixels).

@param dst
    Pointer to the output image.
    \n\b WARNING:
    \li Adequate memory must already be allocated to the image buffer.
    \li Must be 128-byte aligned.

@param dstProps
    Input image properties.

@return
    #FADAS_ERROR_NONE -- Success.

@ingroup cvtyuv
****************************************************************************/
FADAS_API
FadasError_e FadasCvtYUV_DownscaleYUV888AndRGB888( const uint8_t   *src,
                                                   FadasImgProps_t srcProps,
                                                   uint8_t         *dst,
                                                   FadasImgProps_t dstProps );


/************************************************************************//**
@brief
    Scale UYVY image down by 1x to 8x in each dimension, the ratios in
    height and width need not be the same, and convert to RGB888.

@param src
    Pointer to the input image.
    \n\b WARNING: Should be 128-bit aligned.

@param srcProps
    Input image properties.

@param dst
    Pointer to the output image.
    \n\b WARNING: Adequate memory must already be allocated to the image buffer.
    \n\b WARNING: Should be 128-byte aligned.

@param dstProps
    Output image properties.

@return
    #FADAS_ERROR_NONE -- Success.

@ingroup cvtyuv
****************************************************************************/
FADAS_API
FadasError_e FadasCvtYUV_DownscaleUYVYAndRGB888( const uint8_t   *src,
                                                 FadasImgProps_t srcProps,
                                                 uint8_t         *dst,
                                                 FadasImgProps_t dstProps );




/************************************************************************//**
@brief
    Scale YUV888 image down by two in each dimension, the ratios in height
    and width need not be the same, and convert to RGB888. Color conversion
    uses the parameters of FadasCvtYUV_UYVYtoRGB().

@param src
    Pointer to the input image.
    \n\b WARNING: Must be 128-byte aligned.

@param srcProps
    Input image properties.

@param dst
    Pointer to the output image.
    \n\b WARNING:
    \li Adequate memory must already be allocated to the image buffer.
    \li Must be 128-byte aligned.

@param dstProps
    Output image properties.

@return
    #FADAS_ERROR_NONE -- Success.

@ingroup cvtyuv
****************************************************************************/
FADAS_API
FadasError_e FadasCvtYUV_DownscaleYUV888By2AndRGB888( const uint8_t   *src,
                                                      FadasImgProps_t srcProps,
                                                      uint8_t         *dst,
                                                      FadasImgProps_t *dstProps );


/************************************************************************//**
@brief
    Scale UYVY image down by two in each dimension and convert to RGB888.
    Color conversion uses the parameters of FadasCvtYUV_UYVYtoRGB().

@param src
    Pointer to the input image.
    \n\b WARNING: Must be 128-byte aligned.

@param srcProps
    Input image properties.

@param dst
    Pointer to the output image.
    \n\b WARNING:
    \li Adequate memory must already be allocated to the image buffer.
    \li Must be 128-byte aligned.

@param dstProps
    Input image properties.

@return
    #FADAS_ERROR_NONE -- Success.

@ingroup cvtyuv
****************************************************************************/
FADAS_API
FadasError_e FadasCvtYUV_DownscaleUYVYBy2AndRGB888( const uint8_t   *src,
                                                    FadasImgProps_t srcProps,
                                                    uint8_t         *dst,
                                                    FadasImgProps_t *dstProps );


/************************************************************************//**
@brief
    Same as FadasCvtYUV_DownscaleUYVYBy2AndRGB888(), with the additional step of normalization
    at the end

@param src
    Pointer to the input image.
    \n\b WARNING: Must be 128-byte aligned.

@param srcProps
    Input image properties.

@param normlzR
    Image normalization parameters for red channel.
    \n\b WARNING:
    \li normlz.sub \f$(s_r)\f$ has a limited valid range of \f$-255 \le s_r \le 255\f$.
    \li normlz.mul \f$(m_r)\f$ has a limited valid range of \f$-4 \le m_r \le 4\f$.
    \li normlz.add \f$(a_r)\f$ has a limited valid range of \f$-255 \le a_r \le 255\f$.

@param normlzG
    Image normalization parameters for green channel.
    \n\b WARNING:
    \li normlz.sub \f$(s_g)\f$ has a limited valid range of \f$-255 \le s_g \le 255\f$.
    \li normlz.mul \f$(m_g)\f$ has a limited valid range of \f$-4 \le m_g \le 4\f$.
    \li normlz.add \f$(a_g)\f$ has a limited valid range of \f$-255 \le a_g \le 255\f$.

@param normlzB
    Image normalization parameters for blue channel.
    \n\b WARNING:
    \li normlz.sub \f$(s_b)\f$ has a limited valid range of \f$-255 \le s_b \le 255\f$.
    \li normlz.mul \f$(m_b)\f$ has a limited valid range of \f$-4 \le m_b \le 4\f$.
    \li normlz.add \f$(a_b)\f$ has a limited valid range of \f$-255 \le a_b \le 255\f$.

@param dst
    Pointer to the output image.
    \n\b WARNING:
    \li Adequate memory must already be allocated to the image buffer.
    \li Must be 128-byte aligned.

@param dstProps
    Output image properties.

@return
    #FADAS_ERROR_NONE -- Success.

@ingroup cvtyuv
****************************************************************************/
FADAS_API
FadasError_e FadasCvtYUV_DownscaleUYVYBy2ToRGB888AndNormi8( const uint8_t       *src,
                                                            FadasImgProps_t     srcProps,
                                                            FadasNormlzParams_t normlzR,
                                                            FadasNormlzParams_t normlzG,
                                                            FadasNormlzParams_t normlzB,
                                                            int8_t              *dst,
                                                            FadasImgProps_t     *dstProps );

/************************************************************************//**
@brief
    Scale VYUY image down by 1x to 8x in each dimension, the ratios in
    height and width need not be the same, and convert to RGB888.

@param src
    Pointer to the input image.
    \n\b WARNING: Should be 128-bit aligned.

@param srcProps
    Input image properties.

@param dst
    Pointer to the output image.
    \n\b WARNING: Adequate memory must already be allocated to the image buffer.
    \n\b WARNING: Should be 128-byte aligned.

@param dstProps
    Output image properties.

@return
    #FADAS_ERROR_NONE -- Success.

@ingroup cvtyuv
****************************************************************************/
FADAS_API
FadasError_e FadasCvtYUV_DownscaleVYUYAndRGB888( const uint8_t   *src,
                                                 FadasImgProps_t srcProps,
                                                 uint8_t         *dst,
                                                 FadasImgProps_t dstProps );

/************************************************************************//**
@brief
    Scale VYUY image down by two in each dimension and convert to RGB888.
    Color conversion uses parameters of FadasCvtYUV_VYUYtoRGB().

@param src
    Pointer to the input image.
    \n\b WARNING: Must be 128-byte aligned.

@param srcProps
    Input image properties.

@param dst
    Pointer to the output image.
    \n\b WARNING:
    \li Adequate memory must already be allocated to the image buffer.
    \li Must be 128-byte aligned.

@param dstProps
    Output image properties.

@return
    #FADAS_ERROR_NONE -- Success.

@ingroup cvtyuv
****************************************************************************/
FADAS_API
FadasError_e FadasCvtYUV_DownscaleVYUYBy2AndRGB888( const uint8_t   *src,
                                                    FadasImgProps_t srcProps,
                                                    uint8_t         *dst,
                                                    FadasImgProps_t *dstProps );

/************************************************************************//**
@brief
    Scales 8-bit single-channel integer image intensities \f$(y)\f$ by
    subtracting a constant \f$(s)\f$, multiplying by another constant
    \f$(m)\f$, and then adding by another constant \f$(a)\f$.
    \f[
    y' = m (y - s) + a
    \f]
    Normalization typically occurs using patch-based or a globally measured
    mean \f$(s = \mu)\f$ and standard deviation \f$(m = 1/\sigma)\f$.
    \f[
    y' = (y - \mu)/\sigma
    \f]
    Scaling to a new mean \f$(\mu')\f$ and width \f$(\sigma')\f$ can be
    combined into single constants \f$m = \sigma'/\sigma\f$, \f$s = \mu\f$,
    and \f$a = \mu'\f$.
    \f{eqnarray*}{
    y' &=& m (y - s) + a \\
       &=& (\sigma'/\sigma)(y - \mu) + \mu'
    \f}
    Because this routine outputs into 8-bit unsigned integers, values should be
    chosen accordingly. Incoming mean and standard deviation can be
    measured, however the targeted range and bias might be fixed to the requirements
    of the next algorithm in the processing pipeline.  One possible example
    is \f$\sigma'\approx 32\f$ and \f$\mu'\approx 128\f$.

@param src
    Pointer to the input image.
    \n\b WARNING: Must be 128-byte aligned.

@param srcProps
    Input image properties.
    \n\b WARNING: Stride (bytes) must be a multiple of 128.

@param normlz
    Input normalization parameters.
    \n\b WARNING:
    \li normlz.sub \f$(s)\f$ has a limited valid range of \f$-255 \le s \le 255\f$.
    \li normlz.mul \f$(m)\f$ has a limited valid range of \f$-4 \le m \le 4\f$.
    \li normlz.add \f$(a)\f$ has a limited valid range of \f$-255 \le a \le 255\f$.

@param dst
    Pointer to the output image.
    \n\b WARNING:
    \li Adequate memory must already be allocated to the image buffer.
    \li Must be 128-byte aligned.

@param dstStride
    Stride (bytes) for dst [dstStride >= width]

@return
    #FADAS_ERROR_NONE -- Success.

@ingroup cvtyuvh
****************************************************************************/
FADAS_API
FadasError_e FadasCvtYUV_Renormalize( const uint8_t       *__restrict src,
                                      FadasImgProps_t     srcProps,
                                      FadasNormlzParams_t normlz,
                                      uint8_t             *__restrict dst,
                                      uint32_t            dstStride );



/************************************************************************//**
@brief
    Scales 8-bit three-channel color image intensities \f$(y_i)\f$ by subtracting
    a constant \f$(s_i)\f$, multiplying by another constant \f$(m_i)\f$,
    and then adding by another constant \f$(a_i)\f$.
    \f[
    y'_i = m_i (y_i - s_i) + a_i
    \f]
    Normalization typically occurs using patch-based or a globally measured
    mean \f$(s_i = \mu_i)\f$ and standard deviation \f$(m_i = 1/\sigma_i)\f$
    for each color channel.
    \f[
    y'_i = (y_i - \mu_i)/\sigma_i
    \f]
    Scaling to a new mean \f$(\mu'_i)\f$ and width \f$(\sigma'_i)\f$ for
    each color channel can be
    combined into single constants \f$m_i = \sigma'_i/\sigma_i\f$,
    \f$s_i = \mu_i\f$, and \f$a_i = \mu'_i\f$ for each channel.
    \f{eqnarray*}{
    y'_i &=& m_i (y_i - s_i) + a_i \\
         &=& (\sigma'_i/\sigma_i)(y_i - \mu_i) + \mu'_i
    \f}
    Because this routine outputs into 8-bit unsigned integers, values should be
    chosen accordingly. Incoming mean and standard deviation can be
    measured, however the targeted range and bias might be fixed to the requirements
    of the next algorithm in the processing pipeline.  One possible example
    is \f$\sigma'_i\approx 32\f$ and \f$\mu'_i\approx 128\f$.

@param src
    Pointer to the input 8-bit three-channel color image intensities (for example, RGB888).
    \n\b WARNING: Must be 128-byte aligned.

@param srcProps
    Input image properties.
    \n\b WARNING: Stride (bytes) must be a multiple of 128.

@param normlzR
    Image normalization parameters for red channel.
    \n\b WARNING:
    \li normlz.sub \f$(s_r)\f$ has a limited valid range of \f$-255 \le s_r \le 255\f$.
    \li normlz.mul \f$(m_r)\f$ has a limited valid range of \f$-4 \le m_r \le 4\f$.
    \li normlz.add \f$(a_r)\f$ has a limited valid range of \f$-255 \le a_r \le 255\f$.

@param normlzG
    Image normalization parameters for green channel.
    \n\b WARNING:
    \li normlz.sub \f$(s_g)\f$ has a limited valid range of \f$-255 \le s_g \le 255\f$.
    \li normlz.mul \f$(m_g)\f$ has a limited valid range of \f$-4 \le m_g \le 4\f$.
    \li normlz.add \f$(a_g)\f$ has a limited valid range of \f$-255 \le a_g \le 255\f$.

@param normlzB
    Image normalization parameters for blue channel.
    \n\b WARNING:
    \li normlz.sub \f$(s_b)\f$ has a limited valid range of \f$-255 \le s_b \le 255\f$.
    \li normlz.mul \f$(m_b)\f$ has a limited valid range of \f$-4 \le m_b \le 4\f$.
    \li normlz.add \f$(a_b)\f$ has a limited valid range of \f$-255 \le a_b \le 255\f$.

@param dst
    Pointer to the output image in RGB888 format.
    \n\b WARNING:
    \li Adequate memory must already be allocated to the image buffer.
    \li Must be 128-byte aligned.

@param dstStride
    Stride (bytes) for dst [dstStride >= dst_w * 3]

@return
    #FADAS_ERROR_NONE -- Success.

@ingroup cvtyuvh
****************************************************************************/
FADAS_API
FadasError_e FadasCvtYUV_Renormalize888( const uint8_t       *__restrict src,
                                         FadasImgProps_t     srcProps,
                                         FadasNormlzParams_t normlzR,
                                         FadasNormlzParams_t normlzG,
                                         FadasNormlzParams_t normlzB,
                                         uint8_t             *__restrict dst,
                                         uint32_t            dstStride );



/************************************************************************//**
@brief
    Same as FadasCvtYUV_Renormalize888(), except that this function outputs to a signed
    8-bit image.  This output can be used as one layer of a linear-quantized
    int8 NHWC format.

@param src
    Pointer to the input signed 8-bit three-channel image intensities (for example, HWC).
    \n\b WARNING: Must be 128-byte aligned.

@param srcProps
    Input image properties.
    \n\b WARNING: Stride (bytes) must be a multiple of 128.

@param normlzR
    Image normalization parameters for red channel.
    \n\b WARNING:
    \li normlz.sub \f$(s_r)\f$ has a limited valid range of \f$-255 \le s_r \le 255\f$.
    \li normlz.mul \f$(m_r)\f$ has a limited valid range of \f$-4 \le m_r \le 4\f$.
    \li normlz.add \f$(a_r)\f$ has a limited valid range of \f$-255 \le a_r \le 255\f$.

@param normlzG
    Image normalization parameters for green channel.
    \n\b WARNING:
    \li normlz.sub \f$(s_g)\f$ has a limited valid range of \f$-255 \le s_g \le 255\f$.
    \li normlz.mul \f$(m_g)\f$ has a limited valid range of \f$-4 \le m_g \le 4\f$.
    \li normlz.add \f$(a_g)\f$ has a limited valid range of \f$-255 \le a_g \le 255\f$.

@param normlzB
    Image normalization parameters for blue channel.
    \n\b WARNING:
    \li normlz.sub \f$(s_b)\f$ has a limited valid range of \f$-255 \le s_b \le 255\f$.
    \li normlz.mul \f$(m_b)\f$ has a limited valid range of \f$-4 \le m_b \le 4\f$.
    \li normlz.add \f$(a_b)\f$ has a limited valid range of \f$-255 \le a_b \le 255\f$.

@param dst
    Pointer to the output image in RGB888 INT8 HWC format.
    \n\b WARNING:
    \li Adequate memory must already be allocated to the image buffer.
    \li Must be 16-byte aligned.

@param dstStride
    Stride (bytes) for dst [dst_stride >= dst_w * 3 * sizeof(float16_t)]

@ingroup cvtyuvh
****************************************************************************/
FADAS_API
FadasError_e FadasCvtYUV_Renormalize888u8i8( const uint8_t       *__restrict src,
                                             FadasImgProps_t     srcProps,
                                             FadasNormlzParams_t normlzR,
                                             FadasNormlzParams_t normlzG,
                                             FadasNormlzParams_t normlzB,
                                             int8_t              *__restrict  dst,
                                             uint32_t            dstStride );



/************************************************************************//**
@brief
    Same as FadasCvtYUV_Renormalize888() except that this function outputs to a
    floating-point image of the same HWC layout.  This output is appropriate
    for one layer of a NHWC floating-point format.

@param src
    Pointer to the input 8-bit three-channel color image intensities (for example, RGB888).
    \n\b WARNING: Must be 128-byte aligned.

@param srcProps
    Input image properties.
    \n\b WARNING: Stride (bytes) must be a multiple of 128.

@param normlzR
    Image normalization parameters for red channel.
    \n\b WARNING:
    \li normlz.sub \f$(s_r)\f$ has a limited valid range of \f$-255 \le s_r \le 255\f$.
    \li normlz.mul \f$(m_r)\f$ has a limited valid range of \f$-4 \le m_r \le 4\f$.
    \li normlz.add \f$(a_r)\f$ has a limited valid range of \f$-255 \le a_r \le 255\f$.

@param normlzG
    Image normalization parameters for green channel.
    \n\b WARNING:
    \li normlz.sub \f$(s_g)\f$ has a limited valid range of \f$-255 \le s_g \le 255\f$.
    \li normlz.mul \f$(m_g)\f$ has a limited valid range of \f$-4 \le m_g \le 4\f$.
    \li normlz.add \f$(a_g)\f$ has a limited valid range of \f$-255 \le a_g \le 255\f$.

@param normlzB
    Image normalization parameters for blue channel.
    \n\b WARNING:
    \li normlz.sub \f$(s_b)\f$ has a limited valid range of \f$-255 \le s_b \le 255\f$.
    \li normlz.mul \f$(m_b)\f$ has a limited valid range of \f$-4 \le m_b \le 4\f$.
    \li normlz.add \f$(a_b)\f$ has a limited valid range of \f$-255 \le a_b \le 255\f$.

@param dst
    Pointer to the output image in floating-point HWC format.
    \n\b WARNING:
    \li Adequate memory must already be allocated to the image buffer.
    \li Must be 16-byte aligned.

@param dstStride
    Stride (bytes) for dst [dst_stride >= dst_w * 3 * sizeof(float32_t)]

@ingroup cvtyuvh
****************************************************************************/
FADAS_API
FadasError_e FadasCvtYUV_Renormalize888u8f32( const uint8_t       *__restrict src,
                                              FadasImgProps_t     srcProps,
                                              FadasNormlzParams_t normlzR,
                                              FadasNormlzParams_t normlzG,
                                              FadasNormlzParams_t normlzB,
                                              float32_t           *__restrict dst,
                                              uint32_t            dstStride );


/************************************************************************//**
@brief
    Same as FadasCvtYUV_Renormalize888u8f32() except that input for this function is from int8
    instead of uint8.  This output is appropriate for one layer of a NHWC
    floating-point format.

@param src
    Pointer to the input image
    \n\b WARNING: Must be 128-byte aligned.

@param srcProps
    Input image properties.
    \n\b WARNING: Stride (bytes) must be a multiple of 128.

@param normlzR
    Image normalization parameters for red channel.
    \n\b WARNING:
    \li normlz.sub \f$(s_r)\f$ has a limited valid range of \f$-255 \le s_r \le 255\f$.
    \li normlz.mul \f$(m_r)\f$ has a limited valid range of \f$-4 \le m_r \le 4\f$.
    \li normlz.add \f$(a_r)\f$ has a limited valid range of \f$-255 \le a_r \le 255\f$.

@param normlzG
    Image normalization parameters for green channel.
    \n\b WARNING:
    \li normlz.sub \f$(s_g)\f$ has a limited valid range of \f$-255 \le s_g \le 255\f$.
    \li normlz.mul \f$(m_g)\f$ has a limited valid range of \f$-4 \le m_g \le 4\f$.
    \li normlz.add \f$(a_g)\f$ has a limited valid range of \f$-255 \le a_g \le 255\f$.

@param normlzB
    Image normalization parameters for blue channel.
    \n\b WARNING:
    \li normlz.sub \f$(s_b)\f$ has a limited valid range of \f$-255 \le s_b \le 255\f$.
    \li normlz.mul \f$(m_b)\f$ has a limited valid range of \f$-4 \le m_b \le 4\f$.
    \li normlz.add \f$(a_b)\f$ has a limited valid range of \f$-255 \le a_b \le 255\f$.

@param dst
    Pointer to the output image in floating-point HWC format.
    \n\b WARNING:
    \li Adequate memory must already be allocated to the image buffer.
    \li Must be 16-byte aligned.

@param dstStride
    Stride (bytes) for dst [dst_stride >= dst_w * 3 * sizeof(float16_t)]

@ingroup cvtyuvh
****************************************************************************/
FADAS_API
FadasError_e FadasCvtYUV_Renormalize888i8f32( const int8_t        *__restrict src,
                                              FadasImgProps_t     srcProps,
                                              FadasNormlzParams_t normlzR,
                                              FadasNormlzParams_t normlzG,
                                              FadasNormlzParams_t normlzB,
                                              float32_t           *__restrict dst,
                                              uint32_t            dstStride );

/************************************************************************//**
@brief
    Same as FadasCvtYUV_Renormalize888() except that this function outputs to a
    half precision floating-point image of the same HWC layout.  This output is appropriate
    for one layer of a NHWC floating-point format.

@param src
    Pointer to the input image
    \n\b WARNING: Must be 128-byte aligned.

@param srcProps
    Input image properties.
    \n\b WARNING: Stride (bytes) must be a multiple of 128.

@param normlzR
    Image normalization parameters for red channel.
    \n\b WARNING:
    \li normlz.sub \f$(s_r)\f$ has a limited valid range of \f$-255 \le s_r \le 255\f$.
    \li normlz.mul \f$(m_r)\f$ has a limited valid range of \f$-4 \le m_r \le 4\f$.
    \li normlz.add \f$(a_r)\f$ has a limited valid range of \f$-255 \le a_r \le 255\f$.

@param normlzG
    Image normalization parameters for green channel.
    \n\b WARNING:
    \li normlz.sub \f$(s_g)\f$ has a limited valid range of \f$-255 \le s_g \le 255\f$.
    \li normlz.mul \f$(m_g)\f$ has a limited valid range of \f$-4 \le m_g \le 4\f$.
    \li normlz.add \f$(a_g)\f$ has a limited valid range of \f$-255 \le a_g \le 255\f$.

@param normlzB
    Image normalization parameters for blue channel.
    \n\b WARNING:
    \li normlz.sub \f$(s_b)\f$ has a limited valid range of \f$-255 \le s_b \le 255\f$.
    \li normlz.mul \f$(m_b)\f$ has a limited valid range of \f$-4 \le m_b \le 4\f$.
    \li normlz.add \f$(a_b)\f$ has a limited valid range of \f$-255 \le a_b \le 255\f$.

@param dst
    Pointer to the output image in half precision floating-point HWC format.
    \n\b WARNING:
    \li Adequate memory must already be allocated to the image buffer.
    \li Must be 16-byte aligned.

@param dstStride
    Stride (bytes) for dst [dst_stride >= dst_w * 3 * sizeof(float16_t)]

@ingroup cvtyuvh
****************************************************************************/
FADAS_API
FadasError_e FadasCvtYUV_Renormalize888u8f16( const uint8_t       *__restrict src,
                                              FadasImgProps_t     srcProps,
                                              FadasNormlzParams_t normlzR,
                                              FadasNormlzParams_t normlzG,
                                              FadasNormlzParams_t normlzB,
                                              float16_t           *__restrict dst,
                                              uint32_t            dstStride );


#ifdef __cplusplus
}
#endif

#endif /* FADASCVTYUV_H */
