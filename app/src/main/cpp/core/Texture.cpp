//
// Created by Roshan on 12-12-2020.
//

#include "headers/Texture.h"
#include "Renderer.h"
#include "EGL/egl.h"
#include "GLES2/gl2.h"
#include "GLES2/gl2platform.h"
#include "stb_image.h"
#include "android/log.h"

Texture::Texture(std::string path, int slot, int channel, int type) : slot(slot) {
    stbi_set_flip_vertically_on_load(1);
    // load image
    allocationBuffer = stbi_load(path.c_str(), &width, &height, &bpp,
                                 channel); // RGBA for channels so 4 channels

    GLCall(glActiveTexture(GL_TEXTURE0 + slot))
    GLCall(glGenTextures(1, &mReferenceID))
    // Bind reference to texture 2D
    GLCall(glBindTexture(GL_TEXTURE_2D, mReferenceID))
    // These methods need to be called otherwise blank screen will be shown
    GLCall(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR))
    GLCall(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR))
    GLCall(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE))
    GLCall(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE))
    // pass our image to gl
    // gl will see data in RGBA
    // RGBA should be RGBA8 to represent bits per channel
    // we can pass data null instead of mImageLocationBuffer it means we are not ready to load texture in opengl but
    // it created slot for our texture so we can bind in future if till time our texture is not loaded
    // PNG texture tried to load from assets doesn't have alpha channel that's why image is loaded with RGB channel
    // Image loaded to texture with RGB channel
    // RGBA is supported when there are 4 channel's in texture
//    if (type == GL_RGB) {
//        GLCall(glPixelStorei(GL_UNPACK_ALIGNMENT, 1))
//    } else {
//        GLCall(glPixelStorei(GL_UNPACK_ALIGNMENT, 4))
//    }
    GLCall(glTexImage2D(GL_TEXTURE_2D, 0, type, width, height, 0, type, GL_UNSIGNED_BYTE,
                        allocationBuffer))

    // Free image allocation bugger
    if (allocationBuffer) {
        __android_log_print(ANDROID_LOG_DEBUG, "Texture", "width:%i height:%i", width,
                            height);
        stbi_image_free(allocationBuffer);
    } else {
        __android_log_print(ANDROID_LOG_ERROR, "Texture", "Failed");
    }
}

void Texture::bind() {
    GLCall(glBindTexture(GL_TEXTURE_2D, mReferenceID))
}

void Texture::unBind() {
    GLCall(glBindTexture(GL_TEXTURE_2D, 0))
}

Texture::~Texture() {
    GLCall(glDeleteTextures(GL_TEXTURE_2D, &mReferenceID))
}
