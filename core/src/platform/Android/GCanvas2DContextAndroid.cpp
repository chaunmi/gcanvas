/**
 * Created by G-Canvas Open Source Team.
 * Copyright (c) 2017, Alibaba, Inc. All rights reserved.
 *
 * This source code is licensed under the Apache Licence 2.0.
 * For the full copyright and license information, please view
 * the LICENSE file in the root directory of this source tree.
 */

#include "GCanvas2DContextAndroid.h"

#include "GFontCache.h"
#include "GPoint.h"
#include "GFontManagerAndroid.h"
#include "GFrameBufferObject.h"
#include "GShaderManager.h"
#include "support/Log.h"


extern bool g_use_pre_compile;
extern std::string g_shader_cache_path;


GCanvas2DContextAndroid::GCanvas2DContextAndroid(uint32_t w, uint32_t h, GCanvasConfig &config) :
        GCanvasContext(w, h, config, nullptr) {
    Create();
}


GCanvas2DContextAndroid::GCanvas2DContextAndroid(uint32_t w, uint32_t h, GCanvasConfig &config, GCanvasHooks* hooks) :
        GCanvasContext(w, h, config, hooks) {
    Create();
}


void GCanvas2DContextAndroid::Create() {
    mShaderManager = nullptr;

    if (!mConfig.sharedShader) {
        mShaderManager = new GShaderManager();
    }
}


GCanvas2DContextAndroid::~GCanvas2DContextAndroid() {
    if (!mConfig.sharedShader) {
        if (mShaderManager != nullptr) {
            delete mShaderManager;
            mShaderManager = nullptr;
        }
    }
}


void GCanvas2DContextAndroid::SetUseShaderBinaryCache(bool v) {
    g_use_pre_compile = true;
}


void GCanvas2DContextAndroid::SetShaderBinaryCachePath(const std::string& v) {
    g_shader_cache_path = v;
}



void GCanvas2DContextAndroid::InitFBO() {
    if (0 != mContextType) return;

    if (!mConfig.useFbo) {
        return;
    }

    if (!mIsFboSupported) {
        return;
    }

    if (mFboMap.find(DefaultFboName) == mFboMap.end()) {
        std::vector<GCanvasLog> logVec;
        mIsFboSupported = mFboMap[DefaultFboName].InitFBO(mWidth, mHeight,
                                                          GColorTransparent, mEnableFboMsaa,
                                                          &logVec);

        LOG_EXCEPTION_VECTOR(mHooks, mContextId.c_str(), logVec);
    }
}



void GCanvas2DContextAndroid::ClearColorToTransparent()
{
    GColorRGBA c = GColorTransparent;
    ClearColor(c);
}


void GCanvas2DContextAndroid::ClearColor(GColorRGBA& c) {
    glClearColor(c.rgba.r, c.rgba.g, c.rgba.b, c.rgba.a);
    glClear(GL_COLOR_BUFFER_BIT);
}


void GCanvas2DContextAndroid::GetRawImageData(int width, int height, uint8_t *pixels) {
    SendVertexBufferToGPU();
    glReadPixels(0, 0, width, height, GL_RGBA,
                 GL_UNSIGNED_BYTE, pixels);
}


//void GCanvas2DContextAndroid::GLBlend(GCompositeOperation op, GCompositeOperation alphaOp)
//{
//    GBlendOperationFuncs funcs = GCompositeOperationFuncs(op);
//    GBlendOperationFuncs alphaFuncs = GCompositeOperationFuncs(alphaOp);
//
//    glBlendFuncSeparate(funcs.source, funcs.destination,
//                        alphaFuncs.source, alphaFuncs.destination);
//}


void GCanvas2DContextAndroid::BeginDraw(bool is_first_draw) {
    glEnable(GL_DEPTH_TEST);

    if (mConfig.useFbo) {
        // ??????test ????????????
        // glEnable(GL_DEPTH_TEST);
        BindFBO();
    } else {
        if (is_first_draw) {
            // ???fbo?????????????????????????????????????????????
            ClearScreen();
        }
    }
}


void GCanvas2DContextAndroid::EndDraw() {
    if (!mConfig.useFbo) {
        return;
    }

    // FBO??????
    UnbindFBO();
    // ??????fbo???????????????frameBuffer?????? (??????????????????????????????????????????fbo??????)
    ClearScreen();
    // ??????FBO
    DrawFBO(DefaultFboName);
}


GTexture *GCanvas2DContextAndroid::GetFBOTextureData() {
    return &(mFboMap[DefaultFboName].mFboTexture);
}


void GCanvas2DContextAndroid::ResizeCanvas(int width, int height) {
    mWidth = width;
    mHeight = height;

    mCanvasWidth = width;
    mCanvasHeight = height;

    if (mContextType == 0) {
        glViewport(0, 0, width, height);
        glClear(GL_COLOR_BUFFER_BIT | GL_STENCIL_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        mVertexBufferIndex = 0;
        UpdateProjectTransform();

        if (mCurrentState != nullptr) {
            GTransform old = mCurrentState->mTransform;
            mCurrentState->mTransform = GTransformIdentity;
            if (!GTransformEqualToTransform(old, mCurrentState->mTransform)) {
                // update shader transform
                SetTransformOfShader(mProjectTransform);
            }
        }

        ResetStateStack();
        DoSetGlobalCompositeOperation(COMPOSITE_OP_SOURCE_OVER, COMPOSITE_OP_SOURCE_OVER);
        UseDefaultRenderPipeline();

        ClearScreen();
    }

    mFboMap.erase(GCanvasContext::DefaultFboName);
    InitFBO();
    BindFBO();
}



/**
 * ?????????canvas view???????????????????????????
 * ??????fbo, ?????????fbo??????
 */
void GCanvas2DContextAndroid::ResizeCopyUseFbo(int width, int height) {
    bool sizeChanged = mWidth != width || height != mHeight;
    mWidth = width;
    mHeight = height;
    if (!sizeChanged) {
        // LOG_D("execResizeWithGLStatus: not really changed, return");
        return;
    }

    bool shouldChangeDimension = (mCanvasWidth <= 0 && mCanvasHeight <= 0);
    if (0 == mContextType && mIsFboSupported && mWidth > 0 && mHeight > 0) {
        // 1.????????????fbo
        UnbindFBO();
        // 2.?????????fbo
        GFrameBufferObject newFbo;

        // ???????????????init fbo vector?????????
        std::vector<GCanvasLog> logVec;
        mIsFboSupported = newFbo.InitFBO(mWidth, mHeight, GColorTransparent, mEnableFboMsaa, &logVec);
        LOG_EXCEPTION_VECTOR(mHooks, mContextId, logVec);

        // FIXME ???????????????fbo????????????fbo??????????????????????????????????????????

        // ??????????????????fbo, ?????????????????????fbo???????????????
        if (mFboMap.find(DefaultFboName) != mFboMap.end()) {
            // 3.??????fbo
            CopyFBO(mFboMap[DefaultFboName], newFbo);
            // 4.????????????fbo????????????fbo??????
            mFboMap.erase(DefaultFboName);
        }

        // 5.?????????FBO???map???(??????????????????, ??????????????????????????????)
        std::string key = DefaultFboName;
        // ???????????????????????????
        mFboMap.emplace(key, std::move(newFbo));

        // 6.????????????FBO???(????????????)
        BindFBO();
    }

    // ?????????????????????0??????????????????surface??????????????????(0,0)??????dimension???????????????????????????
    if (shouldChangeDimension) {
        SetCanvasDimension(0, 0);
    }

    if (mContextType == 0) {
        // ??????viewport??????
        glViewport(0, 0, mWidth, mHeight);
    }
}


void GCanvas2DContextAndroid::CopyFBO(GFrameBufferObject &srcFbo, GFrameBufferObject &destFbo) {
    if (!mIsFboSupported) {
        return;
    }

    if (nullptr == mCurrentState || nullptr == mCurrentState->mShader) {
        return;
    }

    destFbo.BindFBO();

    ResetGLBeforeCopyFrame(destFbo.mWidth, destFbo.mHeight);

    GColorRGBA color = GColorWhite;
    glBindTexture(GL_TEXTURE_2D, srcFbo.mFboTexture.GetTextureID());
    PushRectangle(-1, -1, 2, 2, 0, 0, 1, 1, color);
    glDrawArrays(GL_TRIANGLES, 0, mVertexBufferIndex);
    mVertexBufferIndex = 0;

    RestoreGLAfterCopyFrame();

    // ????????????????????????fbo
    UnbindFBO();
}


void
GCanvas2DContextAndroid::ResizeCopyUseImage(int width, int height, const unsigned char *rgbaData,
                                            int imgWidth,
                                            int imgHeight) {
    bool sizeChanged = (mWidth != width) || (height != mHeight);
    if (!sizeChanged) {
        // LOGE("sizeChanged not changed");
        return;
    }

    // ????????????
    mWidth = width;
    mHeight = height;

    bool shouldChangeDimension = (mCanvasWidth <= 0 && mCanvasHeight <= 0);
    // ??????????????????
    if (rgbaData != nullptr) {
        CopyImageToCanvas(width, height, rgbaData, imgWidth, imgHeight);
    }

    // ?????????????????????0??????????????????surface??????????????????(0,0)??????dimension???????????????????????????
    if (shouldChangeDimension) {
        SetCanvasDimension(0, 0);
    }
    // ??????viewport??????
    glViewport(0, 0, width, height);
}


void GCanvas2DContextAndroid::CopyImageToCanvas(int width, int height,
                                                const unsigned char *rgbaData, int imgWidth,
                                                int imgHeight) {
    ResetGLBeforeCopyFrame(width, height);
    // ??????????????????
    GLuint glID = BindImage(rgbaData, GL_RGBA, (GLuint) imgWidth, (GLuint) imgHeight);
    GColorRGBA color = GColorWhite;
    PushRectangle(-1, -1, 2, 2, 0, 0, 1, 1, color);
    mCurrentState->mShader->SetTransform(GTransformIdentity);
    glDrawArrays(GL_TRIANGLES, 0, mVertexBufferIndex);
    mVertexBufferIndex = 0;

    glDeleteTextures(1, &glID);

    RestoreGLAfterCopyFrame();
}


void
GCanvas2DContextAndroid::
DrawFBO(std::string fboName, GCompositeOperation compositeOp, float sx, float sy, float sw,
        float sh, float dx, float dy, float dw, float dh) {
    if (!mIsFboSupported) {
        return;
    }

    if (nullptr == mCurrentState || nullptr == mCurrentState->mShader) {
        return;
    }


    Save();
    glViewport(mX, mY, mWidth, mHeight);

    GFrameBufferObject &fbo = mFboMap[fboName];

    UseDefaultRenderPipeline();

    glDisable(GL_STENCIL_TEST);
    glDisable(GL_DEPTH_TEST);

    DoSetGlobalCompositeOperation(compositeOp, compositeOp);

    GColorRGBA color = GColorWhite;
    mCurrentState->mShader->SetOverideTextureColor(0);
    mCurrentState->mShader->SetHasTexture(1);
    fbo.mFboTexture.Bind();

    PushRectangle(-1, -1, 2, 2, 0, 0, 1, 1, color);
    mCurrentState->mShader->SetTransform(GTransformIdentity);
    glDrawArrays(GL_TRIANGLES, 0, mVertexBufferIndex);
    mVertexBufferIndex = 0;

    if (HasClipRegion()) {
        glEnable(GL_STENCIL_TEST);
        glEnable(GL_DEPTH_TEST);
    }

    glViewport(0, 0, mWidth, mHeight);

    Restore();
}


void GCanvas2DContextAndroid::ResetGLBeforeCopyFrame(int width, int height) {
    Save();
    GColorRGBA c = mClearColor;
    SetClearColor(GColorTransparent);
    ClearScreen();
    SetClearColor(c);
    glViewport(0, 0, width, height);

    UseDefaultRenderPipeline();
    glDisable(GL_STENCIL_TEST);
    glDisable(GL_DEPTH_TEST);

    GCompositeOperation compositeOp = COMPOSITE_OP_SOURCE_OVER;
    DoSetGlobalCompositeOperation(compositeOp, compositeOp);

    mCurrentState->mShader->SetOverideTextureColor(0);
    mCurrentState->mShader->SetHasTexture(1);
    mCurrentState->mShader->SetTransform(GTransformIdentity);
}


void GCanvas2DContextAndroid::RestoreGLAfterCopyFrame() {
    if (HasClipRegion()) {
        glEnable(GL_STENCIL_TEST);
    }
    glEnable(GL_DEPTH_TEST);

    Restore();
}


void GCanvas2DContextAndroid::DrawFrame(bool clear) {
    SendVertexBufferToGPU();
    if (clear) {
        ClearGeometryDataBuffers();
    }
}


void GCanvas2DContextAndroid::SetShaderManager(GShaderManager* shaderManager) {
    this->mShaderManager = shaderManager;
}


GShaderManager *GCanvas2DContextAndroid::GetShaderManager() {
    return this->mShaderManager;
}