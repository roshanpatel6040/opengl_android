//
// Created by Roshan on 02-03-2021.
//

#include "headers/CameraView.h"
#include "Renderer.h"

CameraView::CameraView() {
    cameraPos = glm::vec3(0.0f, 0.0f, -12.0f);
    cameraFront = glm::vec3(0.0f, 0.0f, 0.0f);
    cameraUp = glm::vec3(0.0f, 1.0f, 0.0f);
    camera = glm::lookAt(cameraPos, cameraFront, cameraUp);
}

void CameraView::changeCamera(float *orientation, float *distance) {

//    float x = -distance[0];
//    float y = -distance[1];
//    float z = distance[2];
//
//    float yaw = orientation[0];
//    float pitch = orientation[1];
//
//    glm::vec3 direction;
//    direction.x = cos(yaw) * cos(pitch);
//    direction.y = sin(pitch);
//    direction.z = sin(yaw) * cos(pitch);
//    cameraFront = glm::normalize(direction);
//
//    camera = glm::lookAt(cameraPos, cameraPos + cameraFront, cameraUp);
}

void CameraView::useCamera() {
    GLCall(glUniformMatrix4fv(mCameraLocation, 1, false, glm::value_ptr(camera)))
}

void CameraView::setLocation(GLuint location, const char *name) {
    GLCall(mCameraLocation = glGetUniformLocation(location, name))
}

GLuint CameraView::getLocation() {
    return mCameraLocation;
}

CameraView::~CameraView() = default;
