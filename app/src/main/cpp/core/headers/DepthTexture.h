//
// Created by Roshan on 23/07/22.
//

#ifndef OPENGL_DEPTHTEXTURE_H
#define OPENGL_DEPTHTEXTURE_H

#include <GLES2/gl2.h>
#include <GLES2/gl2ext.h>
#include <GLES3/gl31.h>
#include "Renderer.h"
#include "../../../arcore/include/arcore_c_api.h"

class DepthTexture {
public:
    DepthTexture() = default;

    ~DepthTexture() = default;

    void CreateOnGlThread();

    void UpdateWithDepthImageOnGlThread(const ArSession &session, const ArFrame &frame);

    unsigned int GetTextureId() { return texture_id_; }

    unsigned int GetWidth() { return width_; }

    unsigned int GetHeight() { return height_; }

private:
    unsigned int texture_id_ = 0;
    unsigned int width_ = 1;
    unsigned int height_ = 1;

    int64_t previousDepthImageTime = -1;
    int64_t depthImageTime;
};

#endif //OPENGL_DEPTHTEXTURE_H
