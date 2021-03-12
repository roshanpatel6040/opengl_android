//
// Created by Roshan on 02-03-2021.
//

#include "headers/CameraView.h"

CameraView::CameraView() {
    camera = glm::lookAt(glm::vec3(0.0f, 0.0f, 10.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f));
}

void CameraView::changeCamera() {
    camera = glm::lookAt(glm::vec3(0.0f, 0.0f, 10.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f));
}

void CameraView::useCamera() {
    glUniformMatrix4fv(mCameraLocation, 1, false, glm::value_ptr(camera));
}

void CameraView::setLocation(GLuint location, const char *name) {
    mCameraLocation = glGetUniformLocation(location, name);
}

GLuint CameraView::getLocation() {
    return mCameraLocation;
}

CameraView::~CameraView() = default;
