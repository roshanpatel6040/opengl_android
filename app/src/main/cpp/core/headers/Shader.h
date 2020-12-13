//
// Created by Roshan on 12-12-2020.
//
#include <GLES2/gl2.h>

#ifdef __cplusplus
extern "C" {
#endif

#ifndef OPENGL_SHADER_H
#define OPENGL_SHADER_H

class Shader {
private:
    unsigned int mVertexShaderReferenceID;
    unsigned int mFragmentShaderReferenceID;
    GLuint mProgramReferenceID;

public:
    Shader(int program, std::string vertexShaderSource, std::string fragmentShaderSource);

    GLint getAttributeLocation(const char *name);

    void enableVertexAttribArray(GLint index);

    void vertexAttribPointer(GLint positionHandle,int type, int size, int stride, const void *data);

    unsigned int getUniformLocation(const char *name);

    void setUniform4fv(unsigned int location, int size, const float *value);

    void setUniformMatrix4fv(unsigned int location, int size, float *value);

    void setUniform1i(unsigned int location, unsigned int textureId);

    ~Shader();

    void bind();

    void unBind();

    static int loadShader(int type, const std::string& shaderSource);

};

#endif //OPENGL_SHADER_H

#ifdef __cplusplus
}
#endif
