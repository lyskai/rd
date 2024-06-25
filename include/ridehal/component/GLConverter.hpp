// Copyright 2024 Qualcomm Technologies, Inc. All rights reserved.
// Confidential & Proprietary.

#ifndef RIDEHAL_GLConverter_HPP
#define RIDEHAL_GLConverter_HPP

#include <cinttypes>
#include <memory>
#include <unordered_map>
#include <vector>

#define EGL_EGLEXT_PROTOTYPES
#define GL_GLEXT_PROTOTYPES

#include <EGL/egl.h>
#include <EGL/eglext.h>
#include <GLES2/gl2.h>
#include <GLES2/gl2ext.h>
#include <GLES3/gl32.h>

#include <drm/drm_fourcc.h>
#include <gbm.h>
#include <gbm_priv.h>
#include <xf86drm.h>

#include "ridehal/component/ComponentIF.hpp"

namespace ridehal
{
namespace component
{

/*=================================================================================================
** Typedefs
=================================================================================================*/

/** @brief GLConverter Image resolution */
typedef struct
{
    uint32_t width;  /**< Image width */
    uint32_t height; /**< Image height */
} GLConverter_ImageResolution_t;

/** @brief GLConverter ROI Config*/
typedef struct
{
    uint32_t topX;   /**< X coordinate of upper left point */
    uint32_t topY;   /**< Y coordinate of upper left point */
    uint32_t width;  /**< ROI width */
    uint32_t height; /**< ROI height */
} GLConverter_ROIConfig_t;

/** @brief GLConverter Input Configs*/
typedef struct
{
    RideHal_ImageFormat_e inputFormat;             /**< Image format of Input frame */
    GLConverter_ImageResolution_t inputResolution; /**< Image Resolution of Input frame */
    GLConverter_ROIConfig_t ROI;                   /**< Reigion of Interest in Input frame */
} GLConverter_InputConfig_t;

/** @brief GLConverter Component Initialization Configs*/
typedef struct
{
    uint32_t numOfInputs; /**< Number of Input Images in each processing */
    GLConverter_InputConfig_t
            inputConfigs[RIDEHAL_MAX_INPUTS]; /**< Array of Input Configurations */
} GLConverter_Config_t;


/**
 * @brief Component GLConverter
 * @brief GLConverter convert 1 camera frame into another format normalize
 */
class GLConverter final : public ComponentIF
{

    /*=================================================================================================
    ** API Functions
    =================================================================================================*/

public:
    GLConverter();
    ~GLConverter();

    /**
     * @cond GLConverter::Init @endcond
     * @brief Initialize the GLConverter component
     * @param[in] name the component unique instance name
     * @param[in] pConfig the remap configuration paramaters
     * @param[in] level the logger message level
     * @return RIDEHAL_ERROR_NONE on success, others on failure
     */
    RideHalError_e Init( const char *pName, const GLConverter_Config_t *pConfig,
                         Logger_Level_e level = LOGGER_LEVEL_ERROR );

    /**
     * @cond GLConverter::Start @endcond
     * @brief Start the GLConverter pipeline
     * @return RIDEHAL_ERROR_NONE on success, others on failure
     */
    RideHalError_e Start();

    /**
     * @cond GLConverter::Stop @endcond
     * @brief stop the GLConverter pipeline
     * @return RIDEHAL_ERROR_NONE on success, others on failure
     */
    RideHalError_e Stop();

    /**
     * @cond GLConverter::Deinit @endcond
     * @brief deinitialize the GLConverter component
     * @return RIDEHAL_ERROR_NONE on success, others on failure
     */
    RideHalError_e Deinit();

    /**
     * @cond GLConverter::Execute @endcond
     * @brief Execute the GLConverter pipeline
     * @param[in] pInputs the input shared buffers
     * @param[in] numInputs the number of the input shared buffers
     * @param[out] pOutput the output shared buffer
     * @return RIDEHAL_ERROR_NONE on success, others on failure
     */
    RideHalError_e Execute( const RideHal_SharedBuffer_t *pInputs, uint32_t numInputs,
                            const RideHal_SharedBuffer_t *pOutput );


private:
    typedef struct
    {
        gbm_bo *bo;
        EGLImageKHR image;
        GLuint texture;
        GLuint framebuffer;
        uint64_t handle;
        int32_t offset;
    } GL_ImageInfo_t;

    typedef struct
    {
        GLfloat texcoord[4][2];
    } GL_TexCoord_t;

    RideHalError_e EGLInit();

    RideHalError_e CreateGLPipeline();

    RideHalError_e GetInputImageInfo( const RideHal_SharedBuffer_t *pInputBuffer,
                                      std::shared_ptr<GL_ImageInfo_t> &inputInfo );

    RideHalError_e GetOutputImageInfo( const RideHal_SharedBuffer_t *pOutputBuffer,
                                       std::shared_ptr<GL_ImageInfo_t> &outputInfo,
                                       uint32_t batchIdx );

    RideHalError_e Draw( std::shared_ptr<GL_ImageInfo_t> &inputInfo,
                         std::shared_ptr<GL_ImageInfo_t> &outputInfo, uint32_t batchIdx );

    uint32_t GetEGLFormatType( RideHal_ImageFormat_e format );

    uint32_t GetGBMFormatType( RideHal_ImageFormat_e format );


private:
    uint32_t m_numOfInputs = 1;

    bool m_EGLReady = false;
    bool m_GLPipelineReady = false;

    EGLDisplay m_Display = nullptr;
    EGLContext m_Context = nullptr;
    EGLSurface m_Surface = nullptr;
    GLuint m_VertShader = 0;
    GLuint m_FragShader = 0;
    GLuint m_Program = 0;

    GLConverter_ImageResolution_t m_inputResolutions[RIDEHAL_MAX_INPUTS];
    RideHal_ImageFormat_e m_inputFormats[RIDEHAL_MAX_INPUTS];
    GL_TexCoord_t m_textcoords[RIDEHAL_MAX_INPUTS];

    GLConverter_ImageResolution_t m_outputResolution;
    RideHal_ImageFormat_e m_outputFormat;

    static std::mutex s_Lock;
    static int s_DrmDevFd;
    static struct gbm_device *s_GbmDev;
    static uint32_t s_DevRefCnt;

    std::unordered_map<void *, std::shared_ptr<GL_ImageInfo_t>> m_inputImageMap;
    std::unordered_map<void *, std::shared_ptr<GL_ImageInfo_t>> m_outputImageMap;

};   // class GLConverter

}   // namespace component
}   // namespace ridehal

#endif   // RIDEHAL_GLConverter_HPP

