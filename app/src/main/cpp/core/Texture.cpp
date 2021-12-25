//
// Created by Roshan on 12-12-2020.
//

#include <GLES3/gl32.h>
#include "headers/Texture.h"
#include "Renderer.h"
#include "EGL/egl.h"
#include "GLES2/gl2.h"
#include "GLES2/gl2platform.h"
#include "stb_image.h"
#include "android/log.h"

Texture::Texture(std::string path, int slot, int channel, int type) : slot(slot),
                                                                      texturePath(path),
                                                                      textureTarget(GL_TEXTURE_2D) {
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
        __android_log_print(ANDROID_LOG_DEBUG, "Texture", "width:%i height:%i , total channels:%d",
                            width,
                            height, bpp);
        stbi_image_free(allocationBuffer);
    } else {
        __android_log_print(ANDROID_LOG_ERROR, "Texture", "Failed");
    }
}

Texture::Texture(GLenum textureTarget, const std::string path) : textureTarget(textureTarget),
                                                                 texturePath(path) {}

bool Texture::load() {
    stbi_set_flip_vertically_on_load(1);
    allocationBuffer = stbi_load(texturePath.c_str(), &width, &height, &bpp, 0);
    if (!allocationBuffer) {
        printf("Can't load texture from '%s' - %s\n", texturePath.c_str(), stbi_failure_reason());
        exit(0);
    }

    printf("Width %d, height %d, bpp %d\n", width, height, bpp);
    glGenTextures(1, &mReferenceID);
    glBindTexture(textureTarget, mReferenceID);
    if (textureTarget == GL_TEXTURE_2D) {
        switch (bpp) {
            case 1:
                glTexImage2D(textureTarget, 0, GL_RED, width, height, 0, GL_RED, GL_UNSIGNED_BYTE,
                             allocationBuffer);
                break;

            case 3:
                glTexImage2D(textureTarget, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE,
                             allocationBuffer);
                break;

            case 4:
                glTexImage2D(textureTarget, 0, GL_RGBA, width, height, 0, GL_RGBA,
                             GL_UNSIGNED_BYTE, allocationBuffer);
                break;
        }
    } else {
        printf("Support for texture target %x is not implemented\n", textureTarget);
    }
    glTexParameterf(textureTarget, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameterf(textureTarget, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameterf(textureTarget, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameterf(textureTarget, GL_TEXTURE_WRAP_T, GL_REPEAT);

    glBindTexture(textureTarget, 0);
    stbi_image_free(allocationBuffer);

    return true;
}

void Texture::bind() {
    GLCall(glBindTexture(textureTarget, mReferenceID))
}

void Texture::unBind() {
    GLCall(glBindTexture(textureTarget, 0))
}

Texture::~Texture() {
    GLCall(glDeleteTextures(textureTarget, &mReferenceID))
}
