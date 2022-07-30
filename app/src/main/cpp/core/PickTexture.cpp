//
// Created by Roshan on 30/07/22.
//

#include "PickTexture.h"

PickTexture::~PickTexture() {
    if (m_fbo != 0) {
        glDeleteFramebuffers(1, &m_fbo);
    }
    if (m_attachmentTexture1 != 0) {
        glDeleteTextures(1, &m_attachmentTexture1);
    }
    if (m_attachmentTexture2 != 0) {
        glDeleteTextures(1, &m_attachmentTexture2);
    }
    if (m_depthTexture != 0) {
        glDeleteTextures(1, &m_depthTexture);
    }
}


void PickTexture::Init(unsigned int WindowWidth, unsigned int WindowHeight) {

    GLint maxAtt = 0;
    glGetIntegerv(GL_MAX_COLOR_ATTACHMENTS, &maxAtt);
    // LOGE("Pi %d", maxAtt);

    // Create the FBO
    GLCall(glGenFramebuffers(1, &m_fbo))
    GLCall(glBindFramebuffer(GL_FRAMEBUFFER, m_fbo))

    // Create the texture object for the color attachment 1
    GLCall(glGenTextures(1, &m_attachmentTexture1))
    GLCall(glBindTexture(GL_TEXTURE_2D, m_attachmentTexture1))
    GLCall(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST))
    GLCall(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST))
    GLCall(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE))
    GLCall(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE))
    GLCall(glTexImage2D(GL_TEXTURE_2D, 0, GL_R32I, WindowWidth, WindowHeight, 0, GL_RED_INTEGER, GL_INT, nullptr))
    GLCall(glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, m_attachmentTexture1, 0))

    // Create the texture object for the color attachment 2
    GLCall(glGenTextures(1, &m_attachmentTexture2))
    GLCall(glBindTexture(GL_TEXTURE_2D, m_attachmentTexture2))
    GLCall(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST))
    GLCall(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST))
    GLCall(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE))
    GLCall(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE))
    GLCall(glTexImage2D(GL_TEXTURE_2D, 0, GL_R32I, WindowWidth, WindowHeight, 0, GL_RED_INTEGER, GL_INT, nullptr))
    GLCall(glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 + 1, GL_TEXTURE_2D, m_attachmentTexture2, 0))

    // Create the texture object for the color attachment 3
    GLCall(glGenTextures(1, &m_attachmentTexture3))
    GLCall(glBindTexture(GL_TEXTURE_2D, m_attachmentTexture3))
    GLCall(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST))
    GLCall(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST))
    GLCall(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE))
    GLCall(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE))
    GLCall(glTexImage2D(GL_TEXTURE_2D, 0, GL_R32I, WindowWidth, WindowHeight, 0, GL_RED_INTEGER, GL_INT, nullptr))
    GLCall(glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 + 2, GL_TEXTURE_2D, m_attachmentTexture3, 0))

    // Create the texture object for the depth buffer
    GLCall(glGenTextures(1, &m_depthTexture))
    GLCall(glBindTexture(GL_TEXTURE_2D, m_depthTexture))
    GLCall(glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT, WindowWidth, WindowHeight, 0, GL_DEPTH_COMPONENT, GL_FLOAT, nullptr))
    GLCall(glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, m_depthTexture, 0))

    glReadBuffer(GL_NONE);
    const GLenum buffer[2] = {GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1};
    glDrawBuffers(1, buffer);

    // Verify that the FBO is correct
    GLenum Status = glCheckFramebufferStatus(GL_FRAMEBUFFER);

    if (Status != GL_FRAMEBUFFER_COMPLETE) {
        if (Status == GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT) {
            LOGE("Invalid operation Incomplete attachment");
        } else if (Status == GL_FRAMEBUFFER_INCOMPLETE_DIMENSIONS) {
            LOGE("Invalid operation Incomplete dimensions");
        } else if (Status == GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT) {
            LOGE("Invalid operation Incomplete missing attachment");
        } else {
            LOGE("Invalid operation Incomplete other");
        }
        LOGE("Invalid operation %d, %d, %d", WindowWidth, WindowHeight, Status);
    }

    // Restore the default framebuffer
    glBindTexture(GL_TEXTURE_2D, 0);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}


void PickTexture::EnableWriting() {
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, m_fbo);
    const GLenum buffers[3] = {GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1, GL_COLOR_ATTACHMENT2};
    glDrawBuffers(3, buffers);
}

void PickTexture::DisableWriting() {
    // Bind back the default framebuffer
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
}

int PickTexture::ReadPixel(uint32_t attachmentTexture, unsigned int x, unsigned int y) {
    GLCall(glBindFramebuffer(GL_READ_FRAMEBUFFER, m_fbo))
    GLCall(glReadBuffer(GL_COLOR_ATTACHMENT0 + attachmentTexture))
    int Pixel;
    GLCall(glReadPixels(x, y, 1, 1, GL_RED_INTEGER, GL_INT, &Pixel))
    GLCall(glReadBuffer(GL_NONE))
    GLCall(glBindFramebuffer(GL_READ_FRAMEBUFFER, 0))
    return Pixel;
}

