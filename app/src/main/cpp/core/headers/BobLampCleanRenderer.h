//
// Created by Roshan on 24/04/22.
//

#ifndef OPENGL_BOBLAMPCLEANRENDERER_H
#define OPENGL_BOBLAMPCLEANRENDERER_H

#include <GLES/gl.h>
#include "CameraView.h"
#include "Mesh.h"
#include  <string>
#include "Shader.h"

class BobLampCleanRenderer {

private:

    GLuint meshProgram;
    Shader *meshShader;
    CameraView *meshCamera;
    Mesh *mesh;

public :
    void createOnGlThread(AAssetManager *assetManager);

    void Draw(glm::mat4 projection, glm::mat4 cameraView);

    ~BobLampCleanRenderer();

};

#endif //OPENGL_BOBLAMPCLEANRENDERER_H
