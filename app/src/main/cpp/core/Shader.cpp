//
// Created by Roshan on 12-12-2020.
//

#include <string>
#include "Shader.h"
#include "EGL/egl.h"
#include "GLES2/gl2.h"
#include "GLES2/gl2platform.h"
#include "Renderer.h"
#include "android/log.h"

Shader::Shader(){
}

Shader::Shader(int program, std::string vertexShaderSource, std::string fragmentShaderSource)
        : mProgramReferenceID(program) {
    mVertexShaderReferenceID = loadShader(GL_VERTEX_SHADER, vertexShaderSource);
    glAttachShader(program, mVertexShaderReferenceID);
    mFragmentShaderReferenceID = loadShader(GL_FRAGMENT_SHADER, fragmentShaderSource);
    glAttachShader(program, mFragmentShaderReferenceID);
}

void Shader::bind() {
    glAttachShader(mProgramReferenceID, mVertexShaderReferenceID);
    glAttachShader(mProgramReferenceID, mFragmentShaderReferenceID);
}

void Shader::unBind() {
    glDeleteShader(mVertexShaderReferenceID);
    glDeleteShader(mFragmentShaderReferenceID);
}

Shader::~Shader() {
    glDeleteShader(mVertexShaderReferenceID);
    glDeleteShader(mFragmentShaderReferenceID);
}

int Shader::loadShader(int type, const std::string &shaderCode) {
    GLuint shader = glCreateShader(type);
    const char *source = shaderCode.c_str();
    glShaderSource(shader, 1, &source, nullptr);
    glCompileShader(shader);
    GLint vStatus;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &vStatus);
    if (vStatus == GL_FALSE) {
        GLsizei logLength;
        GLchar log[1024];
        __android_log_print(ANDROID_LOG_ERROR, "OpenGL shader status", "%s type: %s", "Failed",
                            (type == GL_VERTEX_SHADER) ? "Vertex" : "Fragment");
        glGetShaderInfoLog(shader, sizeof(log), &logLength, log);
        __android_log_print(ANDROID_LOG_ERROR, "OpenGL shader status", "%s", log);
    }
    return shader;
}

GLint Shader::getAttributeLocation(const char *name) {
    return glGetAttribLocation(mProgramReferenceID, name);
}

unsigned int Shader::getUniformLocation(const char *name) {
    return storeUniformLocation(name);
}

void Shader::enableVertexAttribArray(GLint index) {
    glEnableVertexAttribArray(index);
}

void Shader::vertexAttribPointer(GLint positionHandle, int type, int size, int stride,
                                 const void *data) {
    glVertexAttribPointer(positionHandle, size, type, GL_FALSE, stride, data);
}

void Shader::setUniform4fv(unsigned int location, int size, const float *value) {
    glUniform4fv(location, size, value);
}

void Shader::setUniformMatrix4fv(unsigned int location, int size, float *value) {
    glUniformMatrix4fv(location, size, GL_FALSE, value);
}

void Shader::setUniform1i(unsigned int location, unsigned int textureId) {
    glUniform1i(location, textureId);
}

void Shader::disableVertexAttribPointer(GLint index) {
    glDisableVertexAttribArray(index);
}

unsigned int Shader::storeUniformLocation(const char *name) {
//    if (mOrderedMapUniformLocation.find(name) != mOrderedMapUniformLocation.end()) {
//        return mOrderedMapUniformLocation[name];
//    }
    GLint location = glGetUniformLocation(mProgramReferenceID, name);
//    mOrderedMapUniformLocation[name] = location;
    return location;
}

void Shader::setUniform1iv(unsigned int location, GLint *textureId) {
    glUniform1iv(location, sizeof(textureId), textureId);
}


