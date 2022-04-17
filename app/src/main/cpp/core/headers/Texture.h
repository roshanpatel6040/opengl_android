//
// Created by Roshan on 12-12-2020.
//
#include <string>
#include <GLES2/gl2.h>

#ifdef __cplusplus
extern "C" {
#endif

#ifndef OPENGL_TEXTURE_H
#define OPENGL_TEXTURE_H

class Texture {
private:
    std::string texturePath;
    GLenum textureTarget;
    unsigned int mReferenceID;
    unsigned int slot;
    int width = 0;
    int height = 0;
    int bpp;
    unsigned char *allocationBuffer;

public:
    Texture(std::string path, int slot, int channel, int type);

    Texture(GLenum textureTarget, const std::string path);

    Texture(GLenum textureTarget, const std::string path, int slot);

    bool load();

    void bind();

    void unBind();

    ~Texture();

    inline int getSlot() { return slot; };

    inline int getReferenceId() { return mReferenceID; };
};

#endif //OPENGL_TEXTURE_H

#ifdef __cplusplus
}
#endif