//
// Created by Roshan on 25/04/22.
//

#ifndef OPENGL_CIRCLE_H
#define OPENGL_CIRCLE_H

#include "android/asset_manager.h"
#include "Shader.h"
#include <vector>
#include <glm.hpp>
#include <GLES3/gl3.h>
#include <GLES/gl.h>
#include <ext.hpp>
#include <Texture.h>

class Circle {

public:

    void CreateOnGlThread(AAssetManager *manager);

    void drawCircle(glm::mat4 projection, glm::mat4 camera);

    void drawTriangle(glm::mat4 projection, glm::mat4 camera);

    void drawSquare(glm::mat4 projection, glm::mat4 camera);

    // TODO Create setter methods to change radius

private:
    GLuint program;
    Shader *meshShader;
    int VERTEX_NUM = 45;
    float centerX = 0.0f;
    float centerY = 0.0f;
    float radius = 0.3f;

    std::vector<float> vertices;

};

#endif //OPENGL_CIRCLE_H
