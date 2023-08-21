////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2016-2019 Qualcomm Technologies, Inc.
// All Rights Reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @file  camxchinodedewarp.h
/// @brief node for EIS dewarp
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#ifndef CAMXCHINODEDEWARP_H
#define CAMXCHINODEDEWARP_H

#include <GLES2/gl2.h>
#include <GLES2/gl2ext.h>
#include <GLES2/gl2platform.h>
#include <GLES3/gl3.h>
#include <gl2_types.h>
#include <gralloc_priv.h>
#include <hardware/gralloc1.h>
#include <ui/GraphicBuffer.h>
#include <hardware/hardware.h>
#include <map>
#include <qdMetaData.h>

#include "chiipedefs.h"
#include "camxchinodeutil.h"
#include "chinode.h"
#include "camxosutils.h"
#include "camxcommontypes.h"

#if ANDROID
#include <sys/ioctl.h>
#include <include/linux/ion.h>
#include <linux/dma-buf.h>
//#include <linux/msm_ion.h>
#include <sys/mman.h>               // memory management
#endif
#ifndef ION_SYSTEM_HEAP_ID
#define ION_SYSTEM_HEAP_ID 25
#endif


// NOWHINE FILE CP006: Avoid STL keywordL: map
// NOWHINE FILE DC012: Struct/enum/class members must be documented

/// @brief structure to contain ExtraBuffers vendor tag Id that Dewarp publishes
struct ExtraVendorTags
{
    UINT32 ExtraFrameworkBuffersTagId;    ///< Extra Framework Buffers Tag Id
};

/// @brief enum for Stabilization type of Dewarp node
enum DewarpStabilizationType
{
    DewarpStabilizationTypeNone     = 0,    ///< Stabilization type Nonde
    DewarpStabilizationTypeEISV2    = 2,    ///< Stabilization type EIS2
    DewarpStabilizationTypeEISV3    = 3     ///< Stabilization type EIS3
};

enum DewarpTransformGridSize
{
    TransformGridEISP0size        = (32*1+3)*(24*1+3)*2,    ///< Grid size for precision 0
    TransformGridEISP1size        = (32*2+3)*(24*2+3)*2,    ///< Grid size for precision 1
};
/// @brief EGLconfiguration Attribute sturcture
struct EGLconfigAttrib
{
    INT32    winW;                          ///< Window Width
    INT32    winH;                          ///< Window Height
    INT32    rBits;                         ///< rBits
    INT32    gBits;                         ///< gBits
    INT32    bBits;                         ///< bBits
    INT32    aBits;                         ///< aBits
    INT32    bitsPerPixel;                  ///< bits per pixel
    INT32    depthBits;                     ///< depth bits
    INT32    stencilBits;                   ///< stencil bits
    INT32    sampleBuffers;                 ///< no of sample buffers
    INT32    samples;                       ///< no of samples
};
typedef EGLconfigAttrib EGLCONFIGATTRIB;

/// @brief Buffer structure
struct imgBuffer
{
    struct ion_allocation_data      allocData;          ///< ION allocation data
    INT32                           buffFd;             ///< buffer fd
    INT32                           ionFd;              ///< ION fd
    UINT8*                          pAddr;              ///< virtual addr
    INT32                           cached;             ///< flag indicating cached or not
};
typedef imgBuffer IMGBUFFERT;

/// @brief Image memory handle
struct imgMemHandle
{
    INT32                           bufferFd;           ///< buffer fd
    VOID*                           pVirtualAddr;       ///< buffer virtual addr
    VOID*                           phBufferHandle;     ///< buffer handle
    UINT                            length;             ///< buffer length
};
typedef imgMemHandle IMGMEMHANDLET;

/// @brief Data structure to hold generic data for the img buffer.
struct imgBufferPriv
{
    INT32                     ionFd;          ///< ion device fd
    INT32                     refCount;       ///< ref count for no of clients opening ion device
};
typedef imgBufferPriv IMGBUFFERPRIVT;

class NativeBuffer : public ANativeWindowBuffer, public android::RefBase
{
public:
    void incStrong(const void* id) const { RefBase::incStrong(id); }
    void decStrong(const void* id) const { RefBase::decStrong(id); }

    NativeBuffer(INT32 widthIn, INT32 heightIn, INT32 strideIn, INT32 formatIn,
        INT32 usageIn, native_handle_t *handleIn) :
        ANativeWindowBuffer(), RefBase()
    {
        width           = widthIn;
        height          = heightIn;
        stride          = strideIn;
        format          = formatIn;
        usage           = usageIn;
        handle          = handleIn;
        common.incRef   = incRef;
        common.decRef   = decRef;
    }

private:
    static void incRef(android_native_base_t* base)
    {
        NativeBuffer *self = static_cast<NativeBuffer*>(
            reinterpret_cast<ANativeWindowBuffer*>(base));
        self->incStrong(self);
    }

    static void decRef(android_native_base_t* base)
    {
        NativeBuffer *self = static_cast<NativeBuffer*>(
            reinterpret_cast<ANativeWindowBuffer*>(base));
        self->decStrong(self);
    }
};

/// @brief Data structure for MetaIon buffer Handle
struct metaIonBuffHandle
{
    INT32                   bufferFd;           ///< ion device fd
    VOID*                   pVirtualAddr;       ///< ion device virtual Addr
    VOID*                   phHandle;           ///< ion handle
    UINT                    length;             ///< buffer length
};
typedef metaIonBuffHandle METAIONBUFFHANDLE;

/// @brief Data structure for bufferMap info
struct bufMapInfo
{
    UINT32                  index;                  ///< map Info index
    UINT32                  isInputBuffer;          ///< is input buffer check
    IMGMEMHANDLET           hMetaBuffer;            ///< metabuffer handle
    CSLBufferInfo*          pMetaBufferIon;         ///< meta buffer ion info
    private_handle_t*       phPrivateHandle;        ///< private handle
    NativeBuffer*           pNativeBuffer;          ///< native buffer
    EGLImageKHR             hEGLImage;              ///< egl image
    GLuint                  hColorTexture;          ///< color texture
    CamX::Gralloc*          pGrallocUtils;          ///< gralloc util instance
    GLuint                  hRBO;                   ///< hRBO
    GLuint                  hFBO;                   ///< hFBO
    UCHAR*                  pAddr;                  ///< Coreresponding virtual address to fd
};
typedef bufMapInfo BUFMAPINFO;

struct GPUVendorTags
{
    UINT32 ICAInPerspectiveTransformTagId;  ///< ICA In Perspective Transformation Vendor Tag Id
    UINT32 ICAInGridTransformTagId;         ///< ICA In Grid Transformation Vendor Tag Id
};

/// @brief Structure Request Margins during buffer negotiation
struct MarginRequest
{
    FLOAT widthMargin;   ///< Request margins on the width in percentage (Eg. 0.2 for 20%)
    FLOAT heightMargin;  ///< Request margins on the height in percentage (Eg. 0.1 for 10%)
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @brief Class containing functions to use OpenGL as part of the GPU Node.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class GPUOpenGL
{
public:
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// GetInstance
    ///
    /// @brief  Get instance of the OpenGL class
    ///
    /// @return N/A
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    static GPUOpenGL* GetInstance();

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// DewarpImage
    ///
    /// @brief  Loads the OpenGL shader program from bin file
    ///
    /// @param  hOutput                        pointer where shader code is loaded
    /// @param  hInput                         pointer where shader code is loaded
    /// @param  pIPEICAGridTransform           pointer where shader code is loaded
    ///
    /// @return CDKResult
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CDKResult DewarpImage(
        CHINODEBUFFERHANDLE hOutput,
        CHINODEBUFFERHANDLE hInput,
        FLOAT*              pIPEICAGridTransform);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// Uninitialize
    ///
    /// @brief  Cleans up the resources allocated to use OpenCL with the GPU Node
    ///
    /// @return CDKResultSuccess if success or appropriate error code.
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CDKResult Uninitialize();

    UINT64 m_ReqNum;

private:
    /// @brief enum for GLinit status
    enum GLInitStatus
    {
        GLInitInvalid   = 0,    ///< GlInti is invalid
        GLInitRunning   = 1,    ///< GlInit is running
        GLInitDone      = 2,    ///< GlInit is done
    };

    GLInitStatus m_initStatus;  ///< init status of OpenGL Instance

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// Initialize
    ///
    /// @brief  Initialize the resources required to use OpenGL with the GPU Node
    ///
    /// @return CDKResultSuccess if success or appropriate error code.
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CDKResult Initialize();

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// GPUOpenGL
    ///
    /// @brief  Constructor
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    GPUOpenGL();

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// ~GPUOpenGL
    ///
    /// @brief  Destructor
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    ~GPUOpenGL();

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// InitializeFuncPtrs
    ///
    /// @brief  Loads the OpenGL implementation and initializes the required function pointers
    ///
    /// @return CDKResultSuccess if success or appropriate error code.
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CDKResult InitializeFuncPtrs();

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// CreateContext
    ///
    /// @brief  Creates opengl context
    ///
    /// @param  display        EGL display
    /// @param  sharedContext  EGL shared context
    /// @param  pSurface EGL   surface
    /// @param  pContext EGL   context
    ///
    /// @return CDKResultSuccess    if success or appropriate error code.
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CDKResult CreateContext(
        EGLDisplay   display,
        EGLContext   sharedContext,
        EGLSurface*  pSurface,
        EGLContext*  pContext);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// GetEglConfig
    ///
    /// @brief  Gets egl config
    ///
    /// @param  display      EGL Display
    /// @param  pAttribList  Attribute list
    /// @param  pConfig      EGL config
    ///
    /// @return EGLConfig
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    EGLConfig GetEglConfig(
        EGLDisplay        display,
        const EGLint*     pAttribList,
        EGLCONFIGATTRIB*  pConfig);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// CreateProgram
    ///
    /// @brief  Creates gl program from the shader code
    ///
    /// @param  program         openGL program
    /// @param  vertexShader    vertex shader for openGL
    /// @param  fragmentShader  fragment shader for OpenGL
    ///
    /// @return CDKResult
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CDKResult CreateProgram(
        GLuint    &program,
        GLuint    vertexShader,
        GLuint    fragmentShader);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// CreateShader
    ///
    /// @brief  generates shader from shader code
    ///
    /// @param  shader      openGL shader
    /// @param  shaderType  penGL shader type
    /// @param  pString     openGL program
    ///
    /// @return CDKResult
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CDKResult CreateShader(
        GLuint            &shader,
        GLenum            shaderType,
        const GLchar*     pString);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// CheckEglError
    ///
    /// @brief  Check for egl error
    ///
    /// @param  pOperation  operation name
    ///
    /// @return CDKResult
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CDKResult CheckEglError(
        const CHAR* pOperation);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// CheckglError
    ///
    /// @brief  Checks for gl error
    ///
    /// @param  pOperation  operation name
    ///
    /// @return CDKResult
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CDKResult CheckglError(
        const CHAR* pOperation);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// EnableTextureUnit
    ///
    /// @brief  Enables textureunit
    ///
    /// @param  program           openGL program
    /// @param  pImageUnitString  Image string
    /// @param  unitNum           reference number
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    VOID EnableTextureUnit(
        GLuint          program,
        const CHAR*     pImageUnitString,
        INT32           unitNum);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// LoadProgram
    ///
    /// @brief  Loads opengl program
    ///
    /// @param program  openGL program
    ///
    /// @return CDKResult
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CDKResult LoadProgram(
        GLuint &program);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// GenerateVertexAndTextureCoordinates
    ///
    /// @brief  Generates vertex and texture coordinates
    ///
    /// @param  pVertices        vertex coordinates
    /// @param  pTexCoordinates  texture coordinates
    /// @param  numVerticesX     x-axis points
    /// @param  numVerticesY     y-axis points
    /// @param  widthRatio       width ration
    /// @param  heightRatio      height ratio
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    VOID GenerateVertexAndTextureCoordinates(
        FLOAT*         pVertices,
        FLOAT*         pTexCoordinates,
        INT32          numVerticesX,
        INT32          numVerticesY,
        FLOAT          widthRatio,
        FLOAT          heightRatio);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// GenerateIndices
    ///
    /// @brief  Generates Indices
    ///
    /// @param  pIndices  Indices values
    /// @param  numMeshX  x-axis mesh
    /// @param  numMeshY  y-axis mesh
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    VOID GenerateIndices(
        UINT16*              pIndices,
        INT32                numMeshX,
        INT32                numMeshY);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// AllocIndices
    ///
    /// @brief  Allocates indices for opengl operation
    ///
    /// @param  pIndexVBOID   index pointer
    /// @param  numTriangles  num of triangles
    /// @param  pIndices      indices values
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    VOID AllocIndices(
        GLuint*            pIndexVBOID,
        INT32              numTriangles,
        UINT16*            pIndices);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// GetTransformationMatrices
    ///
    /// @brief  Generates transformation matrix
    ///
    /// @param  pMatrixIn    input matrix
    /// @param  pMatrixOut   output matrix
    /// @param  numVertices  num of vertices
    /// @param  texWidth     texture width
    /// @param  texHeight    texture height
    /// @param  centerX      x coordinate centre
    /// @param  centerY      y coordinate centre
    /// @param  widthRatio   width ratio
    /// @param  heightRatio  height ratio
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    VOID GetTransformationMatrices(
        FLOAT*    pMatrixIn,
        FLOAT*    pMatrixOut,
        INT32     numVertices,
        INT32     texWidth,
        INT32     texHeight,
        INT32     centerX,
        INT32     centerY,
        FLOAT     widthRatio,
        FLOAT     heightRatio);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// ScaleAndCenter
    ///
    /// @brief  Generates scales and center
    ///
    /// @param  pMatrix      input matrix
    /// @param  numVertices  num of vertices
    /// @param  width        width
    /// @param  height       height
    /// @param  centerX      x coordinate of centre
    /// @param  centerY      y coordinate of centre
    /// @param  widthRatio   width ratio
    /// @param  heightRatio  height ratio
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    VOID ScaleAndCenter(
        FLOAT*       pMatrix,
        INT32        numVertices,
        INT32        width,
        INT32        height,
        INT32        centerX,
        INT32        centerY,
        FLOAT        widthRatio,
        FLOAT        heightRatio);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// ConfigureMatrixInfo
    ///
    /// @brief  Configures Matrix info
    ///
    /// @param  pMatrix      input matrix
    /// @param  numVertices  num of vertices
    /// @param  transId      Id
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    VOID ConfigureMatrixInfo(
        FLOAT*     pMatrix,
        INT32      numVertices,
        GLint      transId);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// ConfigureVertexAndTextureCoordinates
    ///
    /// @brief  Configures vertex and texture coordinates
    ///
    /// @param  pVertices        vertices coordinate
    /// @param  pTexCoordinates  texture coordinate
    /// @param  posId            position Id
    /// @param  texId            texture Id
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    VOID ConfigureVertexAndTextureCoordinates(
        FLOAT*       pVertices,
        FLOAT*       pTexCoordinates,
        GLint        posId,
        GLint        texId);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// ConfigureIndices
    ///
    /// @brief  Configures Indices
    ///
    /// @param  pIndexVBOID  Index
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    VOID ConfigureIndices(
        GLuint *pIndexVBOID);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// GetSourceTexture
    ///
    /// @brief  Gets source texture mapping of input buffer
    ///
    /// @param  fd            fd of input buffer
    /// @param  pVirtualAddr  virtual address of input buffer
    /// @param  size          size of input buffer
    /// @param  width         width of input buffer
    /// @param  height        height of input buffer
    /// @param  stride        stride of input buffer
    /// @param  scanline      scanline of input buffer
    ///
    /// @return GLuint
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    GLuint GetSourceTexture(
        UINT32        fd,
        UCHAR*        pVirtualAddr,
        UINT32        size,
        INT32         width,
        INT32         height,
        INT32         stride,
        INT32         scanline);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// GetOutputFrameBuffer
    ///
    /// @brief   Gets sframe buffer mapping of output buffer
    ///
    /// @pararm  fd            fd of input buffer
    /// @param   pVirtualAddr  virtual address of input buffer
    /// @pararm  size          size of input buffer
    /// @pararm  width         width of input buffer
    /// @pararm  height        height of input buffer
    /// @pararm  stride        stride of input buffer
    /// @pararm  scanline      scanline of input buffer
    ///
    /// @return  GLuint
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    GLuint GetOutputFrameBuffer(
        UINT32         fd,
        UCHAR*         pVirtualAddr,
        UINT32         size,
        INT32          width,
        INT32          height,
        INT32          stride,
        INT32          scanline);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// UpdateVertexAndTextureCoordinates
    ///
    /// @brief   Update vertex and texture coordinates
    ///
    /// @pararm  pVertices         Pointer to vertices
    /// @param   pTexCoordinates   Pointer to texture
    /// @param   pGridData         Pointer to grid data
    /// @param   numVerticesX      Num of vertices in X-axis
    /// @param   numVerticesY      Num of vertices in Y-axis
    /// @param   inImageWidth      Input image width
    /// @param   inImageHeight     Input image Height
    /// @param   outImageWidth     Output image width
    /// @param   outImageHeight    Output image Height
    ///
    /// @return  GLuint
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    VOID UpdateVertexAndTextureCoordinates(
        FLOAT*         pVertices,
        FLOAT*         pTexCoordinates,
        const FLOAT*   pGridData,
        INT32          numVerticesX,
        INT32          numVerticesY,
        INT32          inImageWidth,
        INT32          inImageHeight,
        INT32          outImageWidth,
        INT32          outImageHeight);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// FlushBufferMaps
    ///
    /// @brief  Flush all buffer maps
    ///
    /// @return CDKResult
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CDKResult FlushBufferMaps();

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// ImageBufferGet
    ///
    /// @brief   Get image buffer
    ///
    /// @pararm  length    length of buffer
    /// @pararm  phHandle  hanfle of the buffer
    ///
    /// @return  CDKResult
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CDKResult ImageBufferGet(
        INT32               length,
        IMGMEMHANDLET*      phHandle);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// ImageBufferOpen
    ///
    /// @brief  Open Image buffer
    ///
    /// @return INT32
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    INT32 ImageBufferOpen();

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// ImageBufferRelease
    ///
    /// @brief   Release Image buffers
    ///
    /// @pararm  phHandle  handle of the buffer to release
    ///
    /// @return  None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    VOID ImageBufferRelease(
        IMGMEMHANDLET*      phHandle);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// ConvertChiFormat
    ///
    /// @brief  Translate the input Chi Format to the equivalent OpenGL format
    ///
    /// @param  chiFormat Chi Format to be converted
    ///
    /// @return openGLFormat image format type
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    INT32 ConvertChiFormat(
        ChiFormat chiFormat);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// SetPrivateHandleFlags
    ///
    /// @brief  Set private handle flags based on image format.
    ///
    /// @param  chiFormat Chi Format
    ///
    /// @return priv_flags private handle flags
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    INT32 SetPrivateHandleFlags(
        ChiFormat chiFormat);

    typedef void(GL_APIENTRY* PFNGLUSEPROGRAM)(GLuint);
    typedef int(GL_APIENTRY* PFNGLGETATTRIBLOCATION)(GLuint, const GLchar*);
    typedef void(GL_APIENTRY* PFNGLDELETESHADER)(GLuint);
    typedef void(GL_APIENTRY* PFNGLFINISH)(void);
    typedef GLuint(GL_APIENTRY* PFNGLCREATESHADER)(GLenum);
    typedef void(GL_APIENTRY* PFNGLSHADERSOURCE)(GLuint, GLsizei, const GLchar**, const GLint*);
    typedef void(GL_APIENTRY* PFNGLCOMPILESHADER)(GLuint);
    typedef void(GL_APIENTRY* PFNGLGETSHADERIV)(GLuint, GLenum, GLint*);
    typedef void(GL_APIENTRY* PFNGLGETPROGRAMINFOLOG)(GLuint, GLsizei, GLsizei*, GLchar*);
    typedef void(GL_APIENTRY* PFNGLDELETEPROGRAM)(GLuint);
    typedef void(GL_APIENTRY* PFNGLDETACHSHADER)(GLuint, GLuint);
    typedef GLenum(GL_APIENTRY* PFNGLGETERROR)(void);
    typedef int(GL_APIENTRY* PFNGLGETUNIFORMLOCATION)(GLuint, const GLchar*);
    typedef void(GL_APIENTRY* PFNGLACTIVETEXTURE)(GLenum);
    typedef void(GL_APIENTRY* PFNGLUNIFORM1I)(GLint, GLint);
    typedef void(GL_APIENTRY* PFNGLDISABLEVERTEXATTRIBARRAY)(GLuint);
    typedef void(GL_APIENTRY* PFNGLDELETEBUFFERS)(GLsizei n, const GLuint*);
    typedef void(GL_APIENTRY* PFNGLGETPROGRAMIV)(GLuint, GLenum, GLint*);
    typedef void(GL_APIENTRY* PFNGLBINDATTRIBLOCATION)(GLuint, GLuint, const GLchar*);
    typedef void(GL_APIENTRY* PFNGLGETSHADERINFOLOG)(GLuint, GLsizei, GLsizei*, GLchar*);
    typedef GLuint(GL_APIENTRY* PFNGLCREATEPROGRAM)(void);
    typedef void(GL_APIENTRY* PFNGLATTACHSHADER)(GLuint, GLuint);
    typedef void(GL_APIENTRY* PFNGLLINKPROGRAM)(GLuint);
    typedef void(GL_APIENTRY* PFNGLVIEWPORT)(GLint, GLint, GLsizei, GLsizei);
    typedef void(GL_APIENTRY* PFNGLBINDTEXTURE)(GLenum, GLuint);
    typedef void(GL_APIENTRY* PFNGLBINDFRAMEBUFFER)(GLenum, GLuint);
    typedef void(GL_APIENTRY* PFNGLCLEARCOLOR)(GLfloat, GLfloat, GLfloat, GLfloat);
    typedef void(GL_APIENTRY* PFNGLCLEAR)(GLbitfield);
    typedef void(GL_APIENTRY* PFNGLSCISSOR)(GLint, GLint, GLsizei, GLsizei);
    typedef void(GL_APIENTRY* PFNGLBINDBUFFER)(GLenum, GLuint);
    typedef void(GL_APIENTRY* PFNGLENABLEVERTEXATTRIBARRAY)(GLuint);
    typedef void(GL_APIENTRY* PFNGLVERTEXATTRIBPOINTER)(GLuint, GLint, GLenum, GLboolean, GLsizei, const GLvoid*);
    typedef void(GL_APIENTRY* PFNGLGENBUFFERS)(GLsizei n, GLuint*);
    typedef void(GL_APIENTRY* PFNGLBUFFERDATA)(GLenum, GLsizeiptr, const GLvoid*, GLenum);
    typedef void(GL_APIENTRY* PFNGLFLUSH)(void);
    typedef void(GL_APIENTRY* PFNGLDRAWELEMENTS)(GLenum, GLsizei, GLenum, const GLvoid*);
    typedef GLenum(GL_APIENTRY* PFNGLCHECKFRAMEBUFFERSTATUS)(GLenum);
    typedef void(GL_APIENTRY* PFNGLGENTEXTURES)(GLsizei, GLuint*);
    typedef void(GL_APIENTRY* PFNGLTEXPARAMETERI)(GLenum, GLenum, GLint);
    typedef void (GL_APIENTRY* PFNGLEGLIMAGETARGETTEXTURE2DOESPROC) (GLenum, GLeglImageOES);
    typedef void (GL_APIENTRY* PFNGLEGLIMAGETARGETRENDERBUFFERSTORAGEOESPROC) (GLenum, GLeglImageOES);
    typedef void(GL_APIENTRY* PFNGLGENFRAMEBUFFERS)(GLsizei, GLuint*);
    typedef void(GL_APIENTRY* PFNGLGENRENDERBUFFERS)(GLsizei, GLuint*);
    typedef void(GL_APIENTRY* PFNGLBINDRENDERBUFFER)(GLenum, GLuint);
    typedef void(GL_APIENTRY* PFNGLFRAMEBUFFERRENDERBUFFER)(GLenum, GLenum, GLenum, GLuint);
    typedef void(GL_APIENTRY* PFNGLDELETEFRAMEBUFFERS)(GLsizei, const GLuint*);
    typedef void(GL_APIENTRY* PFNGLDELETERENDERBUFFERS)(GLsizei, const GLuint*);
    typedef void(GL_APIENTRY* PFNGLDELETETEXTURES)(GLsizei, const GLuint*);

    typedef EGLDisplay(EGLAPIENTRY* PFNEGLGETDISPLAY)(EGLNativeDisplayType);
    typedef EGLBoolean(EGLAPIENTRY* PFNEGLINITIALIZE)(EGLDisplay, EGLint *, EGLint *);
    typedef EGLBoolean(EGLAPIENTRY* PFNEGLMAKECURRENT)(EGLDisplay, EGLSurface, EGLSurface, EGLContext);
    typedef EGLint(EGLAPIENTRY* PFNEGLGETERROR)(void);
    typedef const char*(EGLAPIENTRY* PFNEGLQUERYSTRING)(EGLDisplay, EGLint);
    typedef EGLContext(EGLAPIENTRY* PFNEGLCREATECONTEXT)(EGLDisplay, EGLConfig, EGLContext, const EGLint *);
    typedef EGLSurface(EGLAPIENTRY* PFNEGLCREATEPBUFFERSURFACE)(EGLDisplay, EGLConfig, const EGLint *);
    typedef EGLBoolean(EGLAPIENTRY* PFNEGLDESTROYCONTEXT)(EGLDisplay, EGLContext);
    typedef EGLBoolean(EGLAPIENTRY* PFNEGLDESTROYSURFACE)(EGLDisplay, EGLSurface);
    typedef EGLBoolean(EGLAPIENTRY* PFNEGLTERMINATE)(EGLDisplay);
    typedef EGLBoolean(EGLAPIENTRY* PFNEGLCHOOSECONFIG)(EGLDisplay, const EGLint *, EGLConfig *, EGLint, EGLint *);
    typedef EGLBoolean(EGLAPIENTRY* PFNEGLGETCONFIGATTRIB)(EGLDisplay, EGLConfig, EGLint, EGLint *);
    typedef EGLBoolean(EGLAPIENTRY* PFNEGLBINDAPI)(EGLenum);
    typedef EGLImageKHR(EGLAPIENTRY* PFNEGLCREATEIMAGEKHR)(EGLDisplay, EGLContext, EGLenum, EGLClientBuffer, const EGLint *);
    typedef EGLDisplay(EGLAPIENTRY* PFNEGLGETCURRENTDISPLAY)(void);
    typedef EGLBoolean(EGLAPIENTRY* PFNEGLDESTROYIMAGEKHRPROC)(EGLDisplay, EGLImageKHR);

    PFNGLUSEPROGRAM                                 m_pfngluseprogram;
    PFNGLGETATTRIBLOCATION                          m_pfnglgetattriblocation;
    PFNGLDELETESHADER                               m_pfngldeleteshader;
    PFNGLFINISH                                     m_pfnglfinish;
    PFNGLCREATESHADER                               m_pfnglcreateshader;
    PFNGLSHADERSOURCE                               m_pfnglshadersource;
    PFNGLCOMPILESHADER                              m_pfnglcompileshader;
    PFNGLGETSHADERIV                                m_pfnglgetshaderiv;
    PFNGLGETPROGRAMINFOLOG                          m_pfnglgetprograminfolog;
    PFNGLDELETEPROGRAM                              m_pfngldeleteprogram;
    PFNGLDETACHSHADER                               m_pfngldetachshader;
    PFNGLGETERROR                                   m_pfnglgeterror;
    PFNGLGETUNIFORMLOCATION                         m_pfnglgetuniformlocation;
    PFNGLACTIVETEXTURE                              m_pfnglactivetexture;
    PFNGLUNIFORM1I                                  m_pfngluniform1i;
    PFNGLDISABLEVERTEXATTRIBARRAY                   m_pfngldisablevertexattribarray;
    PFNGLDELETEBUFFERS                              m_pfngldeletebuffers;
    PFNGLGETPROGRAMIV                               m_pfnglgetprogramiv;
    PFNGLBINDATTRIBLOCATION                         m_pfnglbindattriblocation;
    PFNGLGETSHADERINFOLOG                           m_pfnglgetshaderinfolog;
    PFNGLCREATEPROGRAM                              m_pfnglcreateprogram;
    PFNGLATTACHSHADER                               m_pfnglattachshader;
    PFNGLLINKPROGRAM                                m_pfngllinkprogram;
    PFNGLVIEWPORT                                   m_pfnglviewport;
    PFNGLBINDTEXTURE                                m_pfnglbindtexture;
    PFNGLBINDFRAMEBUFFER                            m_pfnglbindframebuffer;
    PFNGLCLEARCOLOR                                 m_pfnglClearColor;
    PFNGLCLEAR                                      m_pfnglClear;
    PFNGLSCISSOR                                    m_pfnglscissor;
    PFNGLBINDBUFFER                                 m_pfnglbindbuffer;
    PFNGLENABLEVERTEXATTRIBARRAY                    m_pfnglenablevertexattribarray;
    PFNGLVERTEXATTRIBPOINTER                        m_pfnglvertexattribpointer;
    PFNGLGENBUFFERS                                 m_pfnglgenbuffers;
    PFNGLBUFFERDATA                                 m_pfnglbufferdata;
    PFNGLFLUSH                                      m_pfnglflush;
    PFNGLDRAWELEMENTS                               m_pfngldrawelements;
    PFNGLCHECKFRAMEBUFFERSTATUS                     m_pfnglcheckframebufferstatus;
    PFNGLGENTEXTURES                                m_pfnglgentextures;
    PFNGLTEXPARAMETERI                              m_pfngltexparameteri;
    PFNGLEGLIMAGETARGETTEXTURE2DOESPROC             m_pfngleglImageTargetTexture2DOES;
    PFNGLEGLIMAGETARGETRENDERBUFFERSTORAGEOESPROC   m_pfngleglImageTargetRenderbufferStorageOES;
    PFNGLGENFRAMEBUFFERS                            m_pfnglgenframebuffers;
    PFNGLGENRENDERBUFFERS                           m_pfnglgenrenderbuffers;
    PFNGLBINDRENDERBUFFER                           m_pfnglbindrenderbuffer;
    PFNGLFRAMEBUFFERRENDERBUFFER                    m_pfnglframebufferrenderbuffer;
    PFNGLDELETEFRAMEBUFFERS                         m_pfngldeleteframebuffers;
    PFNGLDELETERENDERBUFFERS                        m_pfngldeleterenderbuffers;
    PFNGLDELETETEXTURES                             m_pfngldeletetextures;

    PFNEGLGETDISPLAY                                m_pfneglgetdisplay;
    PFNEGLINITIALIZE                                m_pfneglinitialize;
    PFNEGLMAKECURRENT                               m_pfneglmakecurrent;
    PFNEGLGETERROR                                  m_pfneglgeterror;
    PFNEGLQUERYSTRING                               m_pfneglquerystring;
    PFNEGLCREATECONTEXT                             m_pfneglcreatecontext;
    PFNEGLCREATEPBUFFERSURFACE                      m_pfneglcreatepbuffersurface;
    PFNEGLDESTROYCONTEXT                            m_pfnegldestroycontext;
    PFNEGLDESTROYSURFACE                            m_pfnegldestroysurface;
    PFNEGLTERMINATE                                 m_pfneglterminate;
    PFNEGLCHOOSECONFIG                              m_pfneglchooseconfig;
    PFNEGLGETCONFIGATTRIB                           m_pfneglgetconfigattrib;
    PFNEGLBINDAPI                                   m_pfneglbindapi;
    PFNEGLCREATEIMAGEKHR                            m_pfneglcreateimagekhr;
    PFNEGLGETCURRENTDISPLAY                         m_pfneglgetcurrentdisplay;
    PFNEGLDESTROYIMAGEKHRPROC                       m_pfneglDestroyImageKHR;

    EGLContext                                      m_defaultContext;
    EGLDisplay                                      m_defaultDisplay;
    EGLSurface                                      m_defaultSurface;
    EGLint                                          m_majorVersion;
    EGLint                                          m_minorVersion;
    EGLConfig                                       m_config;
    GLuint                                          m_shaderProgram;
    GLint                                           m_posId;
    GLint                                           m_texId;
    GLint                                           m_transId;
    GLuint                                          m_indexVBOID;
    EGLCONFIGATTRIB                                 m_configDefault;
    FLOAT*                                          m_pVertices;
    FLOAT*                                          m_pTexCoordinates;
    UINT16*                                         m_pIndices;
    INT32                                           m_flags;
    UINT32                                          m_usage;
    INT32                                           m_bufferType;
    INT32                                           m_format;
    FLOAT*                                          m_pTransformationMatrix;
    IMGBUFFERPRIVT*                                 m_pImgBuffInfo;
    INT32                                           m_nNumMeshX;
    INT32                                           m_nNumMeshY;
    INT32                                           m_nNumVerticesX;
    INT32                                           m_nNumVerticesY;
    INT32                                           m_nNumVertices;
    INT32                                           m_PrivatHandleFlags;

    std::map<UINT32, BUFMAPINFO >                   m_SourceTextureMap;
    std::map<UINT32, BUFMAPINFO >                   m_FrameBufferMap;
    CHILIBRARYHANDLE                                m_hOpenGLESV2Lib;
    CHILIBRARYHANDLE                                m_hOpenEGLLib;

    // Program Source from 8998 ppeiscore/QCameraPPEisCore.cpp
    const char *m_pVertexShaderSource = "                                            \n"
                                       "  #version 300 es                            \n"
                                       "  in vec2 inPosition;                        \n"
                                       "  in vec2 inTexCoord;                        \n"
                                       "  out vec2 texCoord;                         \n"
                                       "                                             \n"
                                       " void main()                                 \n"
                                       " {                                           \n"
                                       "     texCoord =  inTexCoord;                 \n"
                                       "     gl_Position = vec4(inPosition,0.0,1.0); \n"
                                       " }";

    const char *m_pFragmentShaderSource = "                                                                    \n"
                                         "#version 300 es                                                      \n"
                                         "#extension GL_EXT_YUV_target : require                               \n"
                                         "precision highp float;                                               \n"
                                         "uniform highp  __samplerExternal2DY2YEXT tex;                        \n"
                                         "in vec2 texCoord;                                                    \n"
                                         "layout (yuv) out vec4 FragColor;                                     \n"
                                         "                                                                     \n"
                                         "void main()                                                          \n"
                                         "{                                                                    \n"
                                         "    FragColor = texture(tex, texCoord);                              \n"
                                         "}";

};

// NOWHINE FILE NC004c: Things outside the Camx namespace should be prefixed with Camx/CSL


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @brief Chi node structure for Dewarp interface.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class ChiDewarpNode
{
public:
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// Initialize
    ///
    /// @brief  Initialization required to create a node
    ///
    /// @param  pCreateInfo  Pointer to a structure that defines create session information for the node.
    ///
    /// @return CDKResultSuccess if success or appropriate error code.
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CDKResult Initialize(
        CHINODECREATEINFO* pCreateInfo);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// QueryBufferInfo
    ///
    /// @brief  Implementation of PFNNODEQUERYBUFFERINFO defined in chinode.h
    ///
    /// @param  pQueryBufferInfo    Pointer to a structure to query the input buffer resolution and type.
    ///
    /// @return CDKResultSuccess if success or appropriate error code.
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CDKResult QueryBufferInfo(
        CHINODEQUERYBUFFERINFO* pQueryBufferInfo);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// SetBufferInfo
    ///
    /// @brief  Implementation of PFNNODESETBUFFERINFO defined in chinode.h
    ///
    /// @param  pSetBufferInfo  Pointer to a structure with information to set the output buffer resolution and type.
    ///
    /// @return CDKResultSuccess if success or appropriate error code.
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CDKResult SetBufferInfo(
        CHINODESETBUFFERPROPERTIESINFO* pSetBufferInfo);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// ProcessRequest
    ///
    /// @brief  Implementation of PFNNODEPROCREQUEST defined in chinode.h
    ///
    /// @param  pProcessRequestInfo Pointer to a structure that defines the information required for processing a request.
    ///
    /// @return CDKResultSuccess if success or appropriate error code.
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CDKResult ProcessRequest(
        CHINODEPROCESSREQUESTINFO* pProcessRequestInfo);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// ChiMemCpyNode
    ///
    /// @brief  Constructor
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    ChiDewarpNode();

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// ~ChiMemCpyNode
    ///
    /// @brief  Destructor
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    virtual ~ChiDewarpNode();

private:
    ChiDewarpNode(const ChiDewarpNode&) = delete;               ///< Disallow the copy constructor
    ChiDewarpNode& operator=(const ChiDewarpNode&) = delete;    ///< Disallow assignment operator

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// GetMetaData
    ///
    /// @brief  Get the needed metadata from camera system
    ///
    /// @param  requestId   The request id for current request
    /// @param  tagId       The id of tag to get from camera system
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    VOID* GetMetaData(
          UINT64 requestId,
          UINT32 tagId);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// GetEisV3Margin
    ///
    /// @brief  Get EIS3Margin
    ///
    /// @return True if the node is bypassable otherwise false
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CDKResult GetEisV3Margin();

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// GetEisV2Margin
    ///
    /// @brief  Get EIS2Margin
    ///
    /// @return True if the node is bypassable otherwise false
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CDKResult GetEisV2Margin();

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// SetStabilizedOutputDimsLookAhead
    ///
    /// @brief  Get whether a node is bypassable or not
    ///
    /// @param  requestId   The request id for current request
    ///
    /// @return True if the node is bypassable otherwise false
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CDKResult SetStabilizedOutputDimsLookAhead(
              UINT32  outputOptimalH,
              INT32   outputOptimalW);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// SetStabilizedOutputDimsRealTime
    ///
    /// @brief  Get whether a node is bypassable or not
    ///
    /// @param  requestId   The request id for current request
    ///
    /// @return True if the node is bypassable otherwise false
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CDKResult SetStabilizedOutputDimsRealTime(
              UINT32  outputOptimalH,
              INT32   outputOptimalW);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// IsByssableNode
    ///
    /// @brief  Method to query if node is bypassable
    ///
    /// @return TRUE or FALSE
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    BOOL IsByssableNode() const
    {
        return m_nodeFlags.isBypassable;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// DewarpImage
    ///
    /// @brief  Dewarp Image
    ///
    /// @param  hOutput              The CHINODEBUFFERHANDLE to output image buffer
    /// @param  hInput               The CHINODEBUFFERHANDLE to output image buffer
    /// @param  pIPEICAGridTransform Transformation Grid
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    VOID DewarpImage(
        CHINODEBUFFERHANDLE hOutput,
        CHINODEBUFFERHANDLE hInput,
        FLOAT*              pIPEICAGridTransform);

    CHIHANDLE                               m_hChiSession;              ///< The Chi session handle
    UINT32                                  m_nodeId;                   ///< The node's Id
    UINT32                                  m_nodeCaps;                 ///< The selected node caps
    CHINODEIMAGEFORMAT                      m_format;                   ///< The selected format
    UINT                                    m_instanceId;               ///< Instance Id

    UINT64                                  m_processedFrame;           ///< The count for processed frame

    CamX::Mutex*                            m_pDewarpNodeMutex;         ///< Dewarp node mutex
    UINT32                                  m_fullOutputDimensions[2];  ///< The output width x height dimension

    GPUOpenGL*                              m_pOpenGL;                  ///< Local GL class instance pointer
    CHINODEFLAGS                            m_nodeFlags;                ///< Node flags
    BOOL                                    m_metaTagsInitialized;
    MarginRequest                           m_EISMarginRequest;
    DewarpStabilizationType                 m_stabilizationType;
    FLOAT                                   m_stabilizationTransformGrid[TransformGridEISP0size];
    GPUVendorTags                           m_vendorTagId = { 0 };
    UINT64                                  m_ReqNum;
    UINT32                                  m_vendorTagBaseIsPreviewStreamPresent;
    FILE*                                   m_hOutputDumpFile;
    BOOL                                    m_bEnableDewarpGridDump;
};

#endif // CAMXCHINODEDEWARP_H
