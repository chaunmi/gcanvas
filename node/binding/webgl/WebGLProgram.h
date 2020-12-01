/**
 * Created by G-Canvas Open Source Team.
 * Copyright (c) 2017, Alibaba, Inc. All rights reserved.
 *
 * This source code is licensed under the Apache Licence 2.0.
 * For the full copyright and license information, please view
 * the LICENSE file in the root directory of this source tree.
 */

#ifndef WEBGLPROGRAM_H
#define WEBGLPROGRAM_H
#include <napi.h>
#include <GL/gl.h>
namespace NodeBinding
{
    class WebGLProgram : public Napi::ObjectWrap<WebGLProgram>
    {
    public:
        WebGLProgram(const Napi::CallbackInfo &info);
        static void Init(Napi::Env env);
        static Napi::Object NewInstance(Napi::Env env, const Napi::Value arg);
        inline GLuint getId() const { return mId; }

    private:
        GLuint mId = 0;
        static Napi::FunctionReference constructor;
    };
} // namespace NodeBinding
#endif