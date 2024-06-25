// Copyright 2024 Qualcomm Technologies, Inc. All rights reserved.
// Confidential & Proprietary.

#include <cinttypes>
#include <cstring>
#include <memory>

#include "ridehal/component/GLConverter.hpp"

namespace ridehal
{
namespace component
{

GLConverter::GLConverter() {}

GLConverter::~GLConverter() {}

inline RideHalError_e GLErrorCheck()
{
    GLenum glError = glGetError();
    return glError == GL_NO_ERROR ? RIDEHAL_ERROR_NONE : RIDEHAL_ERROR_FAIL;
}

static const char *pVertShaderText = "#version 320 es\n"
                                     "layout(location = 0) in vec2 pos;\n"
                                     "layout(location = 1) in vec2 texcoord;\n"
                                     "out vec2 v_texcoord;\n"
                                     "void main() {\n"
                                     "    gl_Position = vec4(pos, 1.0, 1.0);\n"
                                     "    v_texcoord = texcoord;\n"
                                     "}\n";

static const char *pFragShaderText = "#version 320 es\n"
                                     "#extension GL_OES_EGL_image_external : require\n"
                                     "#extension GL_OES_EGL_image_external_essl3 : require\n"
                                     "precision mediump float;\n"
                                     "in vec2 v_texcoord;\n"
                                     "out vec4 color;\n"
                                     "uniform samplerExternalOES tex;\n"
                                     "void main() {\n"
                                     "  color = texture(tex, v_texcoord);\n"
                                     "}\n";

static const char *pFragShaderYUVText = "#version 320 es\n"
                                        "#extension GL_OES_EGL_image_external : require\n"
                                        "#extension GL_OES_EGL_image_external_essl3 : require\n"
                                        "#extension GL_EXT_YUV_target : require\n"
                                        "precision mediump float;\n"
                                        "in vec2 v_texcoord;\n"
                                        "layout(yuv) out vec4 color;\n"
                                        "uniform samplerExternalOES tex;\n"
                                        "void main() {\n"
                                        "  color = texture(tex, v_texcoord);\n"
                                        "}\n";

uint32_t GLConverter::s_DevRefCnt = 0;

RideHalError_e GLConverter::Init( const char *pName, const GLConverter_Config_t *pConfig,
                                  Logger_Level_e level )
{
    RideHalError_e ret = RIDEHAL_ERROR_NONE;

    uint32_t topX = 0;
    uint32_t topY = 0;
    uint32_t roi_width = 0;
    uint32_t roi_height = 0;

    float gl_topX = 0.0f;
    float gl_topY = 0.0f;
    float gl_bottomX = 0.0f;
    float gl_bottomY = 0.0f;

    ret = ComponentIF::Init( pName, level );
    if ( RIDEHAL_ERROR_NONE != ret )
    {
        RIDEHAL_ERROR( "ComponentIF::Init failed" );
    }
    else
    {
        m_state = RIDEHAL_COMPONENT_STATE_INITIALIZING;
        m_numOfInputs = pConfig->numOfInputs;
        if ( m_numOfInputs > RIDEHAL_MAX_INPUTS )
        {
            ret = RIDEHAL_ERROR_OUT_OF_BOUND;
            RIDEHAL_ERROR( "Number of Inputs exceeds maximum limit" );
        }

        if ( RIDEHAL_ERROR_NONE == ret )
        {
            for ( uint32_t i = 0; i < m_numOfInputs; i++ )
            {
                m_inputResolutions[i].width = pConfig->inputConfigs[i].inputResolution.width;
                m_inputResolutions[i].height = pConfig->inputConfigs[i].inputResolution.height;
                m_inputFormats[i] = pConfig->inputConfigs[i].inputFormat;

                topX = pConfig->inputConfigs[i].ROI.topX;
                topY = pConfig->inputConfigs[i].ROI.topY;
                roi_width = pConfig->inputConfigs[i].ROI.width;
                roi_height = pConfig->inputConfigs[i].ROI.height;

                if ( topX >= 0 && topX <= m_inputResolutions[i].width )
                {
                    gl_topX = (float) topX;
                }
                else
                {
                    ret = RIDEHAL_ERROR_BAD_ARGUMENTS;
                    RIDEHAL_ERROR( "ROI topX of input %u is out of range", i );
                    break;
                }

                if ( topY >= 0 && topY <= m_inputResolutions[i].height )
                {
                    gl_topY = (float) topY;
                }
                else
                {
                    ret = RIDEHAL_ERROR_BAD_ARGUMENTS;
                    RIDEHAL_ERROR( "ROI topY of input %u is out of range", i );
                    break;
                }

                if ( roi_width >= 0 && roi_width <= m_inputResolutions[i].width - topX )
                {
                    gl_topX = gl_topX / m_inputResolutions[i].width;
                    gl_bottomX = gl_topX + roi_width;
                }
                else
                {
                    ret = RIDEHAL_ERROR_BAD_ARGUMENTS;
                    RIDEHAL_ERROR( "ROI width of input %u is out of range", i );
                    break;
                }

                if ( roi_height >= 0 && roi_height <= m_inputResolutions[i].height - topY )
                {
                    gl_topY = gl_topY / m_inputResolutions[i].height;
                    gl_bottomY = gl_topY + roi_height;
                }
                else
                {
                    ret = RIDEHAL_ERROR_BAD_ARGUMENTS;
                    RIDEHAL_ERROR( "ROI height of input %u is out of range", i );
                    break;
                }

                GLfloat texcoord[4][2] = { { gl_topX, gl_topY },
                                           { gl_bottomX, gl_topY },
                                           { gl_bottomX, gl_bottomY },
                                           { gl_topX, gl_bottomY } };
                memcpy( m_textcoords[i].texcoord, texcoord, sizeof( texcoord ) );
            }

            std::lock_guard<std::mutex> l( s_Lock );

            if ( RIDEHAL_ERROR_NONE == ret )
            {
                if ( nullptr == s_GbmDev )
                {
                    s_DrmDevFd = drmOpen( "msm_drm", NULL );
                    if ( s_DrmDevFd < 0 )
                    {
                        ret = RIDEHAL_ERROR_FAIL;
                        RIDEHAL_ERROR( "drm open failed: %d", s_DrmDevFd );
                    }
                }
            }

            if ( RIDEHAL_ERROR_NONE == ret )
            {
                s_GbmDev = gbm_create_device( s_DrmDevFd );
                if ( nullptr == s_GbmDev )
                {
                    ret = RIDEHAL_ERROR_FAIL;
                    RIDEHAL_ERROR( "gbm create failed" );
                }
            }

            if ( RIDEHAL_ERROR_NONE == ret )
            {
                s_DevRefCnt++;
            }

            /* Complete initialization */
            if ( RIDEHAL_ERROR_NONE == ret )
            {
                m_state = RIDEHAL_COMPONENT_STATE_READY;
                RIDEHAL_INFO( "Component GLConverter is initialized" );
            }
        }
    }

    return ret;
}

RideHalError_e GLConverter::Start()
{
    RideHalError_e ret = RIDEHAL_ERROR_NONE;
    if ( RIDEHAL_COMPONENT_STATE_READY != m_state )
    {
        ret = RIDEHAL_ERROR_BAD_STATE;
    }

    if ( RIDEHAL_ERROR_NONE == ret )
    {
        // DO start
        m_state = RIDEHAL_COMPONENT_STATE_RUNNING;
        RIDEHAL_INFO( "Component GLConverter start to run" );
    }

    return ret;
}

RideHalError_e GLConverter::Stop()
{
    RideHalError_e ret = RIDEHAL_ERROR_NONE;

    if ( RIDEHAL_COMPONENT_STATE_RUNNING != m_state )
    {
        ret = RIDEHAL_ERROR_BAD_STATE;
    }

    if ( RIDEHAL_ERROR_NONE == ret )
    {
        // DO stop
        m_state = RIDEHAL_COMPONENT_STATE_READY;
        RIDEHAL_INFO( "Component GLConverter is stopped" );
    }

    return ret;
}

RideHalError_e GLConverter::Deinit()
{
    RideHalError_e ret = RIDEHAL_ERROR_NONE;
    EGLBoolean rc = EGL_FALSE;

    if ( RIDEHAL_COMPONENT_STATE_READY != m_state )
    {
        ret = RIDEHAL_ERROR_BAD_STATE;
    }

    std::lock_guard<std::mutex> l( s_Lock );

    if ( RIDEHAL_ERROR_NONE == ret )
    {
        if ( s_DevRefCnt > 0 )
        {
            s_DevRefCnt--;
        }

        for ( auto it = m_inputImageMap.begin(); it != m_inputImageMap.end(); it++ )
        {
            if ( it->second != nullptr )
            {
                if ( it->second->image != nullptr )
                {
                    rc = eglDestroyImageKHR( m_Display, it->second->image );
                    if ( EGL_TRUE != rc )
                    {
                        ret = RIDEHAL_ERROR_FAIL;
                        RIDEHAL_ERROR( "Failed to destroy ImageKHR for input: 0x%x", rc );
                    }
                }

                if ( it->second->bo != nullptr )
                {
                    gbm_bo_destroy( it->second->bo );
                }
            }
        }
        m_inputImageMap.clear();

        for ( auto it = m_outputImageMap.begin(); it != m_outputImageMap.end(); it++ )
        {
            if ( it->second != nullptr )
            {
                if ( it->second->image != nullptr )
                {
                    rc = eglDestroyImageKHR( m_Display, it->second->image );
                    if ( EGL_TRUE != rc )
                    {
                        ret = RIDEHAL_ERROR_FAIL;
                        RIDEHAL_ERROR( "Failed to destroy ImageKHR for output: 0x%x", rc );
                    }
                }

                if ( it->second->bo != nullptr )
                {
                    gbm_bo_destroy( it->second->bo );
                }
            }
        }
        m_outputImageMap.clear();

        glDeleteProgram( m_Program );
        ret = GLErrorCheck();
        if ( RIDEHAL_ERROR_NONE != ret )
        {
            RIDEHAL_ERROR( "Failed to delete GL program" );
        }

        if ( ( s_DevRefCnt == 0 ) && ( s_GbmDev != nullptr ) )
        {
            s_DrmDevFd = drmClose( s_DrmDevFd );
            if ( s_DrmDevFd < 0 )
            {
                ret = RIDEHAL_ERROR_FAIL;
                RIDEHAL_ERROR( "drm close failed: %d", s_DrmDevFd );
            }

            gbm_device_destroy( s_GbmDev );
        }
    }

    ret = ComponentIF::Deinit();
    if ( RIDEHAL_ERROR_NONE == ret )
    {
        /* Complete deinitialization */
        m_state = RIDEHAL_COMPONENT_STATE_INITIAL;
        RIDEHAL_INFO( "Component GLConverter is deinitialized" );
    }
    else
    {
        RIDEHAL_ERROR( "ComponentIF::Deinit failed" );
    }

    return ret;
}

RideHalError_e GLConverter::Execute( const RideHal_SharedBuffer_t *pInputs, uint32_t numInputs,
                                     const RideHal_SharedBuffer_t *pOutput )
{
    RideHalError_e ret = RIDEHAL_ERROR_NONE;

    if ( numInputs != m_numOfInputs )
    {
        ret = RIDEHAL_ERROR_BAD_ARGUMENTS;
        RIDEHAL_ERROR( "Number of inputs not correct: %u != %u", m_numOfInputs, numInputs );
    }

    if ( RIDEHAL_COMPONENT_STATE_RUNNING != m_state )
    {
        ret = RIDEHAL_ERROR_BAD_STATE;
        RIDEHAL_ERROR( "Component GLConverter is not in running state" );
    }

    if ( RIDEHAL_ERROR_NONE == ret )
    {
        for ( size_t i = 0; i < m_numOfInputs; i++ )
        {
            if ( &pInputs[i] == nullptr )
            {
                ret = RIDEHAL_ERROR_INVALID_BUF;
                RIDEHAL_ERROR( "Input buffer %u is null", i );
            }
        }
    }

    if ( RIDEHAL_ERROR_NONE == ret )
    {
        if ( pOutput == nullptr )
        {
            ret = RIDEHAL_ERROR_INVALID_BUF;
            RIDEHAL_ERROR( "Output buffer is null" );
        }
    }


    if ( RIDEHAL_ERROR_NONE == ret )
    {
        ret = EGLInit();
        if ( RIDEHAL_ERROR_NONE != ret )
        {
            RIDEHAL_ERROR( "Failed to Init EGL" );
        }
    }

    if ( RIDEHAL_ERROR_NONE == ret )
    {
        ret = CreateGLPipeline();
        if ( RIDEHAL_ERROR_NONE != ret )
        {
            RIDEHAL_ERROR( "Failed to Create GL Pipeline" );
        }
    }

    if ( RIDEHAL_ERROR_NONE == ret )
    {
        std::shared_ptr<GL_ImageInfo_t> inputInfo = std::make_shared<GL_ImageInfo_t>();
        std::shared_ptr<GL_ImageInfo_t> outputInfo = std::make_shared<GL_ImageInfo_t>();

        for ( size_t i = 0; i < m_numOfInputs; i++ )
        {
            ret = GetInputImageInfo( &pInputs[i], inputInfo );
            if ( RIDEHAL_ERROR_NONE != ret )
            {
                RIDEHAL_ERROR( "Failed to get input image info for input %u: ", i );
                break;
            }

            ret = GetOutputImageInfo( pOutput, outputInfo, i );
            if ( RIDEHAL_ERROR_NONE != ret )
            {
                RIDEHAL_ERROR( "Failed to get output image info for output batch %u: ", i );
                break;
            }

            ret = Draw( inputInfo, outputInfo, i );
            if ( RIDEHAL_ERROR_NONE != ret )
            {
                RIDEHAL_ERROR( "Failed to draw EGL image for index %u: ", i );
                break;
            }
        }
    }

    return ret;
}

RideHalError_e GLConverter::EGLInit()
{
    RideHalError_e ret = RIDEHAL_ERROR_NONE;

    EGLint major = -1;
    EGLint minor = -1;
    EGLint num_config = 0;
    EGLBoolean rc = EGL_FALSE;
    std::vector<EGLConfig> configs;
    const EGLint context_attribs[] = { EGL_CONTEXT_CLIENT_VERSION, 3, EGL_NONE };
    EGLint surface_attribs[] = {
            EGL_WIDTH,  (EGLint) m_outputResolution.width,
            EGL_HEIGHT, (EGLint) m_outputResolution.height,
            EGL_NONE,
    };

    std::lock_guard<std::mutex> l( s_Lock );

    if ( !m_EGLReady )
    {
        m_Display = eglGetPlatformDisplay( EGL_PLATFORM_GBM_KHR, NULL, NULL );
        if ( nullptr == m_Display )
        {
            ret = RIDEHAL_ERROR_FAIL;
            RIDEHAL_ERROR( "Failed to get EGL display" );
        }

        if ( RIDEHAL_ERROR_NONE == ret )
        {
            rc = eglInitialize( m_Display, &major, &minor );
            if ( EGL_TRUE != rc )
            {
                ret = RIDEHAL_ERROR_FAIL;
                RIDEHAL_ERROR( "Failed to initialize EGL: 0x%x", rc );
            }
        }

        if ( RIDEHAL_ERROR_NONE == ret )
        {
            rc = eglGetConfigs( m_Display, NULL, 0, &num_config );
            if ( ( EGL_TRUE != rc ) || ( num_config <= 0 ) )
            {
                ret = RIDEHAL_ERROR_FAIL;
                RIDEHAL_ERROR( "Failed to get config number for EGL: 0x%x", rc );
            }
        }

        if ( RIDEHAL_ERROR_NONE == ret )
        {
            configs.resize( num_config );
            rc = eglGetConfigs( m_Display, configs.data(), num_config, &num_config );
            if ( EGL_TRUE != rc )
            {
                ret = RIDEHAL_ERROR_FAIL;
                RIDEHAL_ERROR( "Failed to get config for EGL: 0x%x", rc );
            }
        }

        if ( RIDEHAL_ERROR_NONE == ret )
        {
            m_Context = eglCreateContext( m_Display, configs[0], EGL_NO_CONTEXT, context_attribs );
            if ( nullptr == m_Context )
            {
                ret = RIDEHAL_ERROR_FAIL;
                RIDEHAL_ERROR( "Failed to create EGL context" );
            }
        }

        if ( RIDEHAL_ERROR_NONE == ret )
        {
            m_Surface = eglCreatePbufferSurface( m_Display, configs[0], surface_attribs );
            if ( nullptr == m_Surface )
            {
                ret = RIDEHAL_ERROR_FAIL;
                RIDEHAL_ERROR( "Failed to create EGL surface" );
            }
        }

        if ( RIDEHAL_ERROR_NONE == ret )
        {
            rc = eglMakeCurrent( m_Display, m_Surface, m_Surface, m_Context );
            if ( EGL_TRUE != rc )
            {
                ret = RIDEHAL_ERROR_FAIL;
                RIDEHAL_ERROR( "Failed to make EGL current: 0x%x", rc );
            }
        }

        m_EGLReady = true;
    }

    return ret;
}

RideHalError_e GLConverter::CreateGLPipeline()
{
    RideHalError_e ret = RIDEHAL_ERROR_NONE;

    std::lock_guard<std::mutex> l( s_Lock );

    if ( !m_GLPipelineReady )
    {
        if ( RIDEHAL_ERROR_NONE == ret )
        {
            m_VertShader = glCreateShader( GL_VERTEX_SHADER );
            if ( 0 == m_VertShader )
            {
                ret = RIDEHAL_ERROR_FAIL;
                RIDEHAL_ERROR( "Failed to create GL Vertex Shader" );
            }
        }

        if ( RIDEHAL_ERROR_NONE == ret )
        {
            glShaderSource( m_VertShader, 1, (char **) &pVertShaderText, NULL );
            ret = GLErrorCheck();
            if ( RIDEHAL_ERROR_NONE != ret )
            {
                RIDEHAL_ERROR( "Failed to set GL Vertex Shader source" );
            }
        }

        if ( RIDEHAL_ERROR_NONE == ret )
        {
            glCompileShader( m_VertShader );
            ret = GLErrorCheck();
            if ( RIDEHAL_ERROR_NONE != ret )
            {
                RIDEHAL_ERROR( "Failed to compile GL Vertex Shader source" );
            }
        }

        if ( RIDEHAL_ERROR_NONE == ret )
        {
            m_FragShader = glCreateShader( GL_FRAGMENT_SHADER );
            if ( 0 == m_FragShader )
            {
                ret = RIDEHAL_ERROR_FAIL;
                RIDEHAL_ERROR( "Failed to create GL Fragment Shader" );
            }
        }

        if ( RIDEHAL_ERROR_NONE == ret )
        {
            if ( ( m_outputFormat == RIDEHAL_IMAGE_FORMAT_NV12 ) ||
                 ( m_outputFormat == RIDEHAL_IMAGE_FORMAT_UYVY ) )
            {
                glShaderSource( m_FragShader, 1, (char **) &pFragShaderYUVText, NULL );
                ret = GLErrorCheck();
                if ( RIDEHAL_ERROR_NONE != ret )
                {
                    RIDEHAL_ERROR( "Failed to set GL Fragment YUV Shader source" );
                }
            }
            else if ( m_outputFormat == RIDEHAL_IMAGE_FORMAT_RGB888 )
            {
                glShaderSource( m_FragShader, 1, (char **) &pFragShaderText, NULL );
                ret = GLErrorCheck();
                if ( RIDEHAL_ERROR_NONE != ret )
                {
                    RIDEHAL_ERROR( "Failed to set GL Fragment Shader source" );
                }
            }
            else
            {
                ret = RIDEHAL_ERROR_BAD_ARGUMENTS;
                RIDEHAL_ERROR( "Unsupported output image format" );
            }
        }

        if ( RIDEHAL_ERROR_NONE == ret )
        {
            glCompileShader( m_FragShader );
            ret = GLErrorCheck();
            if ( RIDEHAL_ERROR_NONE != ret )
            {
                RIDEHAL_ERROR( "Failed to compile GL Fragment Shader source" );
            }
        }

        if ( RIDEHAL_ERROR_NONE == ret )
        {
            m_Program = glCreateProgram();
            ret = GLErrorCheck();
            if ( RIDEHAL_ERROR_NONE != ret )
            {
                RIDEHAL_ERROR( "Failed to create GL Program" );
            }
        }

        if ( RIDEHAL_ERROR_NONE == ret )
        {
            glAttachShader( m_Program, m_VertShader );
            ret = GLErrorCheck();
            if ( RIDEHAL_ERROR_NONE != ret )
            {
                RIDEHAL_ERROR( "Failed to attach GL Vertex Shader" );
            }
        }

        if ( RIDEHAL_ERROR_NONE == ret )
        {
            glAttachShader( m_Program, m_FragShader );
            ret = GLErrorCheck();
            if ( RIDEHAL_ERROR_NONE != ret )
            {
                RIDEHAL_ERROR( "Failed to attach GL Fragment Shader" );
            }
        }

        if ( RIDEHAL_ERROR_NONE == ret )
        {
            glLinkProgram( m_Program );
            ret = GLErrorCheck();
            if ( RIDEHAL_ERROR_NONE != ret )
            {
                RIDEHAL_ERROR( "Failed to link GL program" );
            }
        }

        if ( RIDEHAL_ERROR_NONE == ret )
        {
            glUseProgram( m_Program );
            ret = GLErrorCheck();
            if ( RIDEHAL_ERROR_NONE != ret )
            {
                RIDEHAL_ERROR( "Failed to use GL program" );
            }
        }

        glDeleteShader( m_VertShader );
        glDeleteShader( m_FragShader );

        m_GLPipelineReady = true;
    }

    return ret;
}

RideHalError_e GLConverter::GetInputImageInfo( const RideHal_SharedBuffer_t *pInputBuffer,
                                               std::shared_ptr<GL_ImageInfo_t> &inputInfo )
{
    RideHalError_e ret = RIDEHAL_ERROR_NONE;

    void *bufferAddr = pInputBuffer->data();

    if ( m_inputImageMap.find( bufferAddr ) == m_inputImageMap.end() )
    {
        uint32_t width = pInputBuffer->imgProps.width;
        uint32_t height = pInputBuffer->imgProps.height;
        uint32_t stride = pInputBuffer->imgProps.stride[0];
        RideHal_ImageFormat_e format = pInputBuffer->imgProps.format;
        size_t offset = pInputBuffer->offset;
        uint32_t handle = pInputBuffer->buffer.dmaHandle;


        EGLint eglImageAttribs[] = { EGL_WIDTH,
                                     (EGLint) width,
                                     EGL_HEIGHT,
                                     (EGLint) height,
                                     EGL_LINUX_DRM_FOURCC_EXT,
                                     (EGLint) GetEGLFormatType( format ),
                                     EGL_DMA_BUF_PLANE0_FD_EXT,
                                     (EGLint) handle,
                                     EGL_DMA_BUF_PLANE0_OFFSET_EXT,
                                     (EGLint) offset,
                                     EGL_DMA_BUF_PLANE0_PITCH_EXT,
                                     (EGLint) stride,
                                     EGL_NONE };

        struct gbm_import_fd_data fdData = { (int) handle, width, height, stride,
                                             GetGBMFormatType( format ) };
        inputInfo->bo =
                gbm_bo_import( s_GbmDev, GBM_BO_IMPORT_FD, &fdData, GBM_BO_TRANSFER_READ_WRITE );
        if ( nullptr == inputInfo->bo )
        {
            ret = RIDEHAL_ERROR_FAIL;
            RIDEHAL_ERROR( "Failed to import gbm bo for input" );
        }

        if ( RIDEHAL_ERROR_NONE == ret )
        {
            inputInfo->image = eglCreateImageKHR( m_Display, EGL_NO_CONTEXT, EGL_LINUX_DMA_BUF_EXT,
                                                  NULL, eglImageAttribs );
            if ( nullptr == inputInfo->image )
            {
                ret = RIDEHAL_ERROR_FAIL;
                RIDEHAL_ERROR( "Failed to create image for input" );
            }
        }

        if ( RIDEHAL_ERROR_NONE == ret )
        {
            glGenTextures( 1, &inputInfo->texture );
            ret = GLErrorCheck();
            if ( RIDEHAL_ERROR_NONE != ret )
            {
                RIDEHAL_ERROR( "Failed to generate GL textures for input" );
            }
        }

        if ( RIDEHAL_ERROR_NONE == ret )
        {
            glBindTexture( GL_TEXTURE_EXTERNAL_OES, inputInfo->texture );
            ret = GLErrorCheck();
            if ( RIDEHAL_ERROR_NONE != ret )
            {
                RIDEHAL_ERROR( "Failed to bind GL textures for input" );
            }
        }

        if ( RIDEHAL_ERROR_NONE == ret )
        {
            glEGLImageTargetTexture2DOES( GL_TEXTURE_EXTERNAL_OES, inputInfo->image );
            ret = GLErrorCheck();
            if ( RIDEHAL_ERROR_NONE != ret )
            {
                RIDEHAL_ERROR( "Failed to render GL target texture for input" );
            }
        }

        if ( RIDEHAL_ERROR_NONE == ret )
        {
            inputInfo->handle = handle;
            inputInfo->offset = offset;
            m_inputImageMap[bufferAddr] = inputInfo;
        }
    }
    else
    {
        inputInfo = m_inputImageMap[bufferAddr];
    }

    return ret;
}

RideHalError_e GLConverter::GetOutputImageInfo( const RideHal_SharedBuffer_t *pOutputBuffer,
                                                std::shared_ptr<GL_ImageInfo_t> &outputInfo,
                                                uint32_t batchIdx )
{
    RideHalError_e ret = RIDEHAL_ERROR_NONE;

    uint32_t outputSize = pOutputBuffer->size / m_numOfInputs;
    void *bufferAddr = (void *) ( (uint8_t *) pOutputBuffer->data() + batchIdx * outputSize );

    if ( m_outputImageMap.find( bufferAddr ) == m_outputImageMap.end() )
    {
        uint32_t width = pOutputBuffer->imgProps.width;
        uint32_t height = pOutputBuffer->imgProps.height;
        uint32_t stride = pOutputBuffer->imgProps.stride[0];
        RideHal_ImageFormat_e format = pOutputBuffer->imgProps.format;
        size_t offset = pOutputBuffer->offset;
        uint32_t handle = pOutputBuffer->buffer.dmaHandle;

        EGLint eglImageAttribs[] = { EGL_WIDTH,
                                     (EGLint) width,
                                     EGL_HEIGHT,
                                     (EGLint) height,
                                     EGL_LINUX_DRM_FOURCC_EXT,
                                     (EGLint) GetEGLFormatType( format ),
                                     EGL_DMA_BUF_PLANE0_FD_EXT,
                                     (EGLint) handle,
                                     EGL_DMA_BUF_PLANE0_OFFSET_EXT,
                                     (EGLint) offset,
                                     EGL_DMA_BUF_PLANE0_PITCH_EXT,
                                     (EGLint) stride,
                                     EGL_NONE };

        struct gbm_import_fd_data fdData = { (int) handle, width, height, stride,
                                             GetGBMFormatType( format ) };
        outputInfo->bo =
                gbm_bo_import( s_GbmDev, GBM_BO_IMPORT_FD, &fdData, GBM_BO_TRANSFER_READ_WRITE );
        if ( nullptr == outputInfo->bo )
        {
            ret = RIDEHAL_ERROR_FAIL;
            RIDEHAL_ERROR( "Failed to import gbm bo for output" );
        }

        if ( RIDEHAL_ERROR_NONE == ret )
        {
            outputInfo->image = eglCreateImageKHR( m_Display, EGL_NO_CONTEXT, EGL_LINUX_DMA_BUF_EXT,
                                                   NULL, eglImageAttribs );
            if ( nullptr == outputInfo->image )
            {
                ret = RIDEHAL_ERROR_FAIL;
                RIDEHAL_ERROR( "Failed to create image for output" );
            }
        }

        if ( RIDEHAL_ERROR_NONE == ret )
        {
            glGenTextures( 1, &outputInfo->texture );
            ret = GLErrorCheck();
            if ( RIDEHAL_ERROR_NONE != ret )
            {
                RIDEHAL_ERROR( "Failed to generate GL texture for input" );
            }
        }

        if ( RIDEHAL_ERROR_NONE == ret )
        {
            glGenFramebuffers( 1, &outputInfo->framebuffer );
            ret = GLErrorCheck();
            if ( RIDEHAL_ERROR_NONE != ret )
            {
                RIDEHAL_ERROR( "Failed to generate GL frame buffers for input" );
            }
        }

        if ( RIDEHAL_ERROR_NONE == ret )
        {
            glBindTexture( GL_TEXTURE_EXTERNAL_OES, outputInfo->texture );
            ret = GLErrorCheck();
            if ( RIDEHAL_ERROR_NONE != ret )
            {
                RIDEHAL_ERROR( "Failed to bind GL texture for input" );
            }
        }

        if ( RIDEHAL_ERROR_NONE == ret )
        {
            glEGLImageTargetTexture2DOES( GL_TEXTURE_EXTERNAL_OES, outputInfo->image );
            ret = GLErrorCheck();
            if ( RIDEHAL_ERROR_NONE != ret )
            {
                RIDEHAL_ERROR( "Failed to render GL target texture for output" );
            }
        }

        if ( RIDEHAL_ERROR_NONE == ret )
        {
            glBindFramebuffer( GL_FRAMEBUFFER, outputInfo->framebuffer );
            ret = GLErrorCheck();
            if ( RIDEHAL_ERROR_NONE != ret )
            {
                RIDEHAL_ERROR( "Failed to bind GL frame buffer for output" );
            }
        }

        if ( RIDEHAL_ERROR_NONE == ret )
        {
            glFramebufferTexture2D( GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_EXTERNAL_OES,
                                    outputInfo->texture, 0 );
            ret = GLErrorCheck();
            if ( RIDEHAL_ERROR_NONE != ret )
            {
                RIDEHAL_ERROR( "Failed to attach GL texture to output buffer" );
            }
        }

        if ( RIDEHAL_ERROR_NONE == ret )
        {
            outputInfo->handle = handle;
            outputInfo->offset = offset;
            m_outputImageMap[bufferAddr] = outputInfo;
        }
    }
    else
    {
        outputInfo = m_outputImageMap[bufferAddr];
    }

    return ret;
}

RideHalError_e GLConverter::Draw( std::shared_ptr<GL_ImageInfo_t> &inputInfo,
                                  std::shared_ptr<GL_ImageInfo_t> &outputInfo, uint32_t batchIdx )
{
    RideHalError_e ret = RIDEHAL_ERROR_NONE;

    glViewport( 0, 0, (GLint) m_outputResolution.width, (GLint) m_outputResolution.height );
    ret = GLErrorCheck();
    if ( RIDEHAL_ERROR_NONE != ret )
    {
        RIDEHAL_ERROR( "glViewport failed" );
    }

    if ( RIDEHAL_ERROR_NONE == ret )
    {
        glClearColor( 1.0, 0.0, 0.0, 1.0 );
        ret = GLErrorCheck();
        if ( RIDEHAL_ERROR_NONE != ret )
        {
            RIDEHAL_ERROR( "glClearColor failed" );
        }
    }

    if ( RIDEHAL_ERROR_NONE == ret )
    {
        glClear( GL_COLOR_BUFFER_BIT );
        ret = GLErrorCheck();
        if ( RIDEHAL_ERROR_NONE != ret )
        {
            RIDEHAL_ERROR( "glClear failed" );
        }
    }

    if ( RIDEHAL_ERROR_NONE == ret )
    {
        glUniform1i( glGetUniformLocation( m_Program, "tex" ), 0 );
        ret = GLErrorCheck();
        if ( RIDEHAL_ERROR_NONE != ret )
        {
            RIDEHAL_ERROR( "glUniform1i failed" );
        }
    }

    if ( RIDEHAL_ERROR_NONE == ret )
    {
        // bind input
        glBindTexture( GL_TEXTURE_EXTERNAL_OES, inputInfo->texture );
        glEGLImageTargetTexture2DOES( GL_TEXTURE_EXTERNAL_OES, inputInfo->image );
        ret = GLErrorCheck();
        if ( RIDEHAL_ERROR_NONE != ret )
        {
            RIDEHAL_ERROR( "Failed to bind input" );
        }
    }

    if ( RIDEHAL_ERROR_NONE == ret )
    {
        // bind output
        glBindFramebuffer( GL_FRAMEBUFFER, outputInfo->framebuffer );
        glFramebufferTexture2D( GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_EXTERNAL_OES,
                                outputInfo->texture, 0 );
        ret = GLErrorCheck();
        if ( RIDEHAL_ERROR_NONE != ret )
        {
            RIDEHAL_ERROR( "Failed to bind output" );
        }
    }

    GLushort indices[] = { 0, 1, 3, 3, 1, 2 };
    GLfloat pos[4][2] = { { -1.0, -1.0 }, { 1.0, -1.0 }, { 1.0, 1.0 }, { -1.0, 1.0 } };
    if ( RIDEHAL_ERROR_NONE == ret )
    {
        glVertexAttribPointer( glGetAttribLocation( m_Program, "pos" ), 2, GL_FLOAT, GL_FALSE, 0,
                               pos );
        ret = GLErrorCheck();
        if ( RIDEHAL_ERROR_NONE != ret )
        {
            RIDEHAL_ERROR( "Failed to set GL vertex pos" );
        }
    }

    if ( RIDEHAL_ERROR_NONE == ret )
    {
        glVertexAttribPointer( glGetAttribLocation( m_Program, "texcoord" ), 2, GL_FLOAT, GL_FALSE,
                               0, m_textcoords[batchIdx].texcoord );
        ret = GLErrorCheck();
        if ( RIDEHAL_ERROR_NONE != ret )
        {
            RIDEHAL_ERROR( "Failed to set GL vertex texcoord" );
        }
    }

    if ( RIDEHAL_ERROR_NONE == ret )
    {
        glEnableVertexAttribArray( glGetAttribLocation( m_Program, "pos" ) );
        ret = GLErrorCheck();
        if ( RIDEHAL_ERROR_NONE != ret )
        {
            RIDEHAL_ERROR( "Failed to enable GL vertex pos" );
        }
    }

    if ( RIDEHAL_ERROR_NONE == ret )
    {
        glEnableVertexAttribArray( glGetAttribLocation( m_Program, "texcoord" ) );
        ret = GLErrorCheck();
        if ( RIDEHAL_ERROR_NONE != ret )
        {
            RIDEHAL_ERROR( "Failed to enable GL vertex texcoord" );
        }
    }

    if ( RIDEHAL_ERROR_NONE == ret )
    {
        glDrawElements( GL_TRIANGLES, sizeof( indices ) / sizeof( indices[0] ), GL_UNSIGNED_SHORT,
                        indices );
        ret = GLErrorCheck();
        if ( RIDEHAL_ERROR_NONE != ret )
        {
            RIDEHAL_ERROR( "Failed to draw GL elements" );
        }
    }

    if ( RIDEHAL_ERROR_NONE == ret )
    {
        glFlush();
        ret = GLErrorCheck();
        if ( RIDEHAL_ERROR_NONE != ret )
        {
            RIDEHAL_ERROR( "Failed to flush" );
        }
    }

    if ( RIDEHAL_ERROR_NONE == ret )
    {
        eglSwapBuffers( m_Display, m_Surface );
        ret = GLErrorCheck();
        if ( RIDEHAL_ERROR_NONE != ret )
        {
            RIDEHAL_ERROR( "Failed to swap GL buffers" );
        }
    }

    return ret;
}


uint32_t GLConverter::GetEGLFormatType( RideHal_ImageFormat_e format )
{
    uint32_t eglFormat = (uint32_t) RIDEHAL_IMAGE_FORMAT_MAX;
    switch ( format )
    {
        case RIDEHAL_IMAGE_FORMAT_UYVY:
            eglFormat = DRM_FORMAT_UYVY;
            break;
        case RIDEHAL_IMAGE_FORMAT_NV12:
            eglFormat = DRM_FORMAT_NV12;
            break;
        case RIDEHAL_IMAGE_FORMAT_P010:
            eglFormat = DRM_FORMAT_P010;
            break;
        case RIDEHAL_IMAGE_FORMAT_RGB888:
            eglFormat = DRM_FORMAT_RGB888;
            break;
        default:
            RIDEHAL_ERROR( "Unsupported EGL image format" );
            break;
    }

    return eglFormat;
}

uint32_t GLConverter::GetGBMFormatType( RideHal_ImageFormat_e format )
{
    uint32_t gbmFormat = (uint32_t) RIDEHAL_IMAGE_FORMAT_MAX;
    switch ( format )
    {
        case RIDEHAL_IMAGE_FORMAT_UYVY:
            gbmFormat = GBM_FORMAT_UYVY;
            break;
        case RIDEHAL_IMAGE_FORMAT_NV12:
            gbmFormat = GBM_FORMAT_NV12;
            break;
        case RIDEHAL_IMAGE_FORMAT_P010:
            gbmFormat = GBM_FORMAT_P010;
            break;
        case RIDEHAL_IMAGE_FORMAT_RGB888:
            gbmFormat = GBM_FORMAT_RGB888;
            break;
        default:
            RIDEHAL_ERROR( "Unsupported GBM image format" );
            break;
    }

    return gbmFormat;
}

}   // namespace component
}   // namespace ridehal

