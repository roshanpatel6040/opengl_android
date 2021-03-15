//
// Created by Roshan on 02-03-2021.
//

#include "headers/CameraView.h"

CameraView::CameraView() {
    cameraPos = glm::vec3(0.0f, 0.0f, 10.0f);
    cameraFront = glm::vec3(0.0f, 0.0f, 0.0f);
    cameraUp = glm::vec3(0.0f, 1.0f, 0.0f);
    camera = glm::lookAt(cameraPos, cameraFront, cameraUp);
}

void CameraView::changeCamera(float *orientations) {

//    const float cameraSpeed = 0.05f; // adjust accordingly
//    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
//        cameraPos += cameraSpeed * cameraFront;
//    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
//        cameraPos -= cameraSpeed * cameraFront;
//    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
//        cameraPos -= glm::normalize(glm::cross(cameraFront, cameraUp)) * cameraSpeed;
//    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
//        cameraPos += glm::normalize(glm::cross(cameraFront, cameraUp)) * cameraSpeed;

    float yaw = orientations[0];
    float pitch = orientations[1];

    glm::vec3 direction;
    direction.x = cos(yaw) * cos(pitch);
    direction.y = -sin(pitch);
    direction.z = sin(yaw) * cos(pitch);
    cameraFront = glm::normalize(direction);

    camera = glm::lookAt(cameraPos, cameraPos + cameraFront, cameraUp);
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
