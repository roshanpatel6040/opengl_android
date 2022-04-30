//
// Created by Roshan on 24/04/22.
//

#include <android/log.h>
#include "BobLampCleanRenderer.h"

#include "Renderer.h"

void BobLampCleanRenderer::createOnGlThread(AAssetManager *assetManager) {
    GLCall(meshProgram = glCreateProgram())

    AAsset *vertAsset = AAssetManager_open(assetManager, "shaders/anim_object.vert",
                                           AASSET_MODE_BUFFER);
    size_t vertLength = AAsset_getLength(vertAsset);
    char *vertexSource = (char *) malloc((sizeof(char) * vertLength) + 1);
    AAsset_read(vertAsset, vertexSource, vertLength);
    AAsset_close(vertAsset);
    vertexSource += '\0';
    AAsset *fragAsset = AAssetManager_open(assetManager, "shaders/anim_object.frag",
                                           AASSET_MODE_BUFFER);
    size_t fragLength = AAsset_getLength(fragAsset);
    char *fragSource = (char *) malloc((sizeof(char) * fragLength) + 1);
    AAsset_read(fragAsset, fragSource, fragLength);
    AAsset_close(fragAsset);
    fragSource += '\0';

    meshShader = new Shader(meshProgram, vertexSource, fragSource);
    GLCall(glLinkProgram(meshProgram))
    delete fragSource;
    delete vertexSource;

    GLboolean isMeshProgram = glIsProgram(meshProgram);
    if (isMeshProgram == GL_FALSE) {
        __android_log_print(ANDROID_LOG_ERROR, "OpenGL isProgram", "%s", "Failed");
    }
    GLint meshStatus;
    glGetProgramiv(meshProgram, GL_LINK_STATUS, &meshStatus);
    if (meshStatus == GL_FALSE) {
        __android_log_print(ANDROID_LOG_ERROR, "OpenGL program status", "%s", "Failed");
        GLsizei logLength;
        GLchar log[1024];
        glGetProgramInfoLog(meshProgram, sizeof(log), &logLength, log);
        __android_log_print(ANDROID_LOG_ERROR, "OpenGL program status", "%s", log);
    }

    mesh = new Mesh();
    bool isLoaded = mesh->LoadMesh("boblampclean.md5mesh");
    __android_log_print(ANDROID_LOG_ERROR, "Model loading", "loaded %d", isLoaded);

    meshCamera = new CameraView();
    GLCall(meshCamera->setLocation(meshProgram, "camera"))
    GLCall(glBindBuffer(GL_ARRAY_BUFFER,0))
    // GLCall(glBindBuffer(GL_ELEMENT_ARRAY_BUFFER,0))
}

void BobLampCleanRenderer::Draw(glm::mat4 projection, glm::mat4 cameraView) {
    GLCall(glUseProgram(meshProgram))
    glm::mat4 translate = glm::translate(glm::mat4(1.0), glm::vec3(0.0f, 0.0f, -10.0f));
    glm::mat4 rotationX = glm::rotate(glm::mat4(1.0f), glm::radians(270.0f),
                                      glm::vec3(1.0f, 0.0f, 0.0f));
    glm::mat4 rotationY = glm::rotate(glm::mat4(1.0f), glm::radians(0.0f),
                                      glm::vec3(0.0f, 0.0f, 1.0f));
    glm::mat4 scale = glm::scale(glm::mat4(1.0), glm::vec3(0.05, 0.05, 0.05));
    glm::mat4 model = translate * rotationX * rotationY * scale;
    GLCall(GLuint projectionLocation = meshShader->getUniformLocation("projection"))
    GLCall(GLuint modelLocation = meshShader->getUniformLocation("model"))
    meshCamera->useCamera(cameraView);

    std::vector<Matrix4f> boneMatrix;
    mesh->GetBoneTransforms(mesh->getAnimationSecond(), boneMatrix);

    GLuint mBoneLocation[100];
    for (unsigned int i = 0; i < ARRAY_SIZE_IN_ELEMENTS(mBoneLocation); i++) {
        char Name[128];
        memset(Name, 0, sizeof(Name));
        snprintf(Name, sizeof(Name), "gBones[%d]", i);
        GLCall(mBoneLocation[i] = meshShader->getUniformLocation(Name))
    }

    for (int i = 0; i < boneMatrix.size(); ++i) {
        if (i >= 100) {
            break;
        }
        Matrix4f matrix = boneMatrix[i];
        GLCall(glUniformMatrix4fv(mBoneLocation[i], 1, GL_TRUE, (const GLfloat *) matrix))
    }

    // GLCall(GLuint meshTextureLocation = meshShader->getUniformLocation("texture"))
    GLCall(meshShader->setUniformMatrix4fv(projectionLocation, 1, glm::value_ptr(projection)))
    GLCall(meshShader->setUniformMatrix4fv(modelLocation, 1, glm::value_ptr(model)))

    Vector3f localPos = mesh->GetWorldTransform().WorldPosToLocalPos(
            meshCamera->getCameraPos());
    GLCall(GLuint localCameraPosLocation = meshShader->getUniformLocation("gCameraLocalPos"))
    GLCall(glUniform3f(localCameraPosLocation, localPos.x, localPos.y, localPos.z))

    mesh->Render(meshProgram);
}

BobLampCleanRenderer::~BobLampCleanRenderer() {
    delete meshShader;
    delete mesh;
    delete meshCamera;
}