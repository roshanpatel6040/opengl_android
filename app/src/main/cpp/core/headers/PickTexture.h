//
// Created by Roshan on 30/07/22.
//

#ifndef OPENGL_PICKTEXTURE_H
#define OPENGL_PICKTEXTURE_H

#include "GLES2/gl2.h"
#include "GLES3/gl31.h"
#include "Renderer.h"

class PickTexture {

public:
    PickTexture() {}

    ~PickTexture();

    void Init(unsigned int WindowWidth, unsigned int WindowHeight);

    void EnableWriting();

    void DisableWriting();

    struct PixelInfo {
        int ObjectID = 0;
        // int DrawID = 0;
        // int PrimID = 0;
    };

    int ReadPixel(uint32_t attachmentTexture, unsigned int x, unsigned int y);

private:
    GLuint m_fbo = 0;
    GLuint m_attachmentTexture1 = 0;
    GLuint m_attachmentTexture2 = 0;
    GLuint m_attachmentTexture3 = 0;
    GLuint m_depthTexture = 0;
};

#endif //OPENGL_PICKTEXTURE_H
