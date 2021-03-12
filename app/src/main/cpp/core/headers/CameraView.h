//
// Created by Roshan on 02-03-2021.
//

#ifndef OPENGL_CAMERAVIEW_H
#define OPENGL_CAMERAVIEW_H

#include <ext.hpp>
#include <string>
#include "GLES2/gl2.h"
#include "glm.hpp"
#include "string.h"

#include "stdlib.h"

class CameraView {

public:
    CameraView();

    void setLocation(GLuint program, const char *name);

    void changeCamera();

    void useCamera();

    GLuint getLocation();

    ~CameraView();

private:
    GLuint mCameraLocation{};
    glm::mat4 camera{};
};

#endif //OPENGL_CAMERAVIEW_H
