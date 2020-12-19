//
// Created by Roshan
//

#include <cstdio>
#include <jni.h>
#include <EGL/egl.h>
#include <GLES/gl.h>
#include <GLES/glplatform.h>
#include <GLES2/gl2.h>
#include <GLES2/gl2platform.h>
#include <GLES2/gl2ext.h>
#include <string>
#include <android/log.h>
#include <iostream>
#include <cassert>
#include "stb_image/stb_image.cpp"
#include "glm/glm.hpp"
#include "glm/ext.hpp"
#include <android/asset_manager_jni.h>
#include <android/asset_manager.h>
#include "core/headers/Renderer.h"
#include "core/headers/Shader.h"
#include "core/headers/Texture.h"
#include "core/headers/VertexBuffer.h"
#include "camera/NdkCameraCaptureSession.h"
#include "camera/NdkCameraDevice.h"
#include "camera/NdkCameraError.h"
#include "camera/NdkCameraManager.h"
#include "camera/NdkCameraMetadata.h"
#include "camera/NdkCameraMetadataTags.h"
#include "Camera/NdkCaptureRequest.h"
#include "camera/NdkCameraWindowType.h"
#include "android/native_window.h"
#include "android/native_activity.h"
#include "android/native_window_jni.h"
#include "media/NdkImageReader.h"
#include "media/NdkImage.h"
#include "media/NdkMediaFormat.h"
#include "thread"

using namespace std;

int windowWidth = 0;
int windowHeight = 0;
ACaptureSessionOutput *captureSessionOutput;
ACaptureSessionOutputContainer *captureSessionOutputContainer;
int textureID;
GLuint arrayBuffer[2];
GLuint program;
ACaptureRequest *previewRequest;
ACameraManager *cameraManager;

#define Camera(x) \
    if(x != ACAMERA_OK){ \
        __android_log_print(ANDROID_LOG_ERROR,"Camera status","Line: %i, error: %i",__LINE__,x); \
    } \


static void onDisconnected(void *context, ACameraDevice *device) {
    __android_log_print(ANDROID_LOG_ERROR, "Camera", "%s", "Disconnected");
}

static void onError(void *context, ACameraDevice *device, int error) {
    __android_log_print(ANDROID_LOG_ERROR, "Camera", "%i", error);
}

static void onActive(void *context, ACameraCaptureSession *session) {
    __android_log_print(ANDROID_LOG_ERROR, "Camera session", "%s", "active");
//    ACameraCaptureSession_setRepeatingRequest(session, nullptr, 1, &previewRequest, nullptr);
}

static void onClosed(void *context, ACameraCaptureSession *session) {
    __android_log_print(ANDROID_LOG_ERROR, "Camera session", "%s", "onClosed");
}

static ACameraDevice_StateCallbacks stateCallbacks = {
        .context = nullptr,
        .onDisconnected = onDisconnected,
        .onError = onError,
};

static ACameraCaptureSession_stateCallbacks sessionCallbacks = {
        .context = nullptr,
        .onActive = onActive,
        .onClosed = onClosed,
};

static void imageCallback(void *context, AImageReader *reader) {
    AImage *image = nullptr;
    auto status = AImageReader_acquireNextImage(reader, &image);
    // Check status here ...

    // Try to process data without blocking the callback
    std::thread processor([=]() {

        uint8_t *data = nullptr;
        int len = 0;
        AImage_getPlaneData(image, 0, &data, &len);

        // Process data here
        // ...

        AImage_delete(image);
    });
    processor.detach();
}

AImageReader *createJpegReader() {
    AImageReader *reader = nullptr;
    media_status_t status = AImageReader_new(680, 480, AIMAGE_FORMAT_JPEG,
                                             2, &reader);

    if (status != AMEDIA_OK) {
        // Handle errors here
    }

    AImageReader_ImageListener listener{
            .context = nullptr,
            .onImageAvailable = imageCallback,
    };

    AImageReader_setImageListener(reader, &listener);

    return reader;
}

ANativeWindow *createSurface(AImageReader *reader) {
    ANativeWindow *nativeWindow;
    AImageReader_getWindow(reader, &nativeWindow);

    return nativeWindow;
}

void startCamera(JNIEnv *env, jobject surface) {
    ACameraIdList *cameraIds = nullptr;
    ACameraManager_getCameraIdList(cameraManager, &cameraIds);
    std::string backFacingId;

    for (int i = 0; i < cameraIds->numCameras; ++i) {
        const char *id = cameraIds->cameraIds[i];
        ACameraMetadata *metaData = nullptr;
        ACameraManager_getCameraCharacteristics(cameraManager, id, &metaData);

        ACameraMetadata_const_entry lensInfo = {0};
        ACameraMetadata_getConstEntry(metaData, ACAMERA_LENS_FACING, &lensInfo);

        auto facing = static_cast<acamera_metadata_enum_android_lens_facing_t>(lensInfo.data.u8[0]);
        if (facing == ACAMERA_LENS_FACING_BACK) {
            backFacingId = id;
            break;
        }
    }
    ACameraManager_deleteCameraIdList(cameraIds);
    ACameraDevice *cameraDevice;
    // Open camera
    camera_status_t openCameraStatus = ACameraManager_openCamera(cameraManager,
                                                                 backFacingId.c_str(),
                                                                 &stateCallbacks, &cameraDevice);

    Camera(openCameraStatus)

    camera_status_t captureRequestStatus = ACameraDevice_createCaptureRequest(cameraDevice,
                                                                              TEMPLATE_PREVIEW,
                                                                                &previewRequest);
    Camera(captureRequestStatus)

    // Create window with surface
    ANativeWindow *window = ANativeWindow_fromSurface(env, surface);

    // Prepare outputs for session
    camera_status_t captureSessionOutputStatus = ACaptureSessionOutput_create(window,
                                                                              &captureSessionOutput);
    Camera(captureSessionOutputStatus)
    camera_status_t captureSessionOutputContainerStatus = ACaptureSessionOutputContainer_create(
            &captureSessionOutputContainer);
    Camera(captureSessionOutputContainerStatus)
    camera_status_t captureSessionOutputContainerAddStatus = ACaptureSessionOutputContainer_add(
            captureSessionOutputContainer, captureSessionOutput);
    Camera(captureSessionOutputContainerAddStatus)

    ANativeWindow_acquire(window);
    ACameraOutputTarget *outputTarget;
    camera_status_t outputTargetStatus = ACameraOutputTarget_create(window, &outputTarget);
    Camera(outputTargetStatus)
    camera_status_t addTargetStatus = ACaptureRequest_addTarget(previewRequest, outputTarget);
    Camera(addTargetStatus)

    ACameraCaptureSession *session;
    camera_status_t captureSessionStatus = ACameraDevice_createCaptureSession(cameraDevice,
                                                                              captureSessionOutputContainer,
                                                                              &sessionCallbacks,
                                                                              &session);
    Camera(captureSessionStatus)
    camera_status_t repeatingRequestStatus = ACameraCaptureSession_setRepeatingRequest(session,
                                                                                       nullptr, 1,
                                                                                       &previewRequest,
                                                                                       nullptr);
    Camera(repeatingRequestStatus)
}

extern "C" {
void Java_com_demo_opengl_CameraActivity_initialize(JNIEnv *jni, jobject object) {
    cameraManager = ACameraManager_create();
}

void Java_com_demo_opengl_CameraActivity_destroy(JNIEnv *jni, jobject object) {
    ACameraManager_delete(cameraManager);
}

void Java_com_demo_opengl_CameraActivity_00024GL_00024Render_onSurfaceCreated(JNIEnv *jni,
                                                                              jobject object,
                                                                              jint textureId,
                                                                              jobject surface) {
    textureID = textureId;

    float vertices[] = {
            -1.0f, -1.0f, 0.0f, 0.0f, 0.0f,
            -1.0, 1.0f, 0.0f, 0.0f, 1.0f,
            1.0f, 1.0f, 0.0f, 1.0f, 0.0f,
            -1.0f, 1.0f, 0.0f, 1.0f, 1.0f
    };

    unsigned int indices[] = {
            2, 1, 0, 0, 3, 2
    };

    std::string vertexShaderCode = "attribute vec3 position;\n"
                                   "attribute vec2 texChords;\n"
                                   "uniform mat4 texMatrix;\n"
                                   "uniform mat4 u_MVP;\n"
                                   "varying vec2 v_Chord;\n"
                                   "void main()\n"
                                   "{\n"
                                   "v_Chord = (texMatrix * vec4(texChords.x, texChords.y, 0, 0)).xy;\n"
                                   "gl_Position = u_MVP * vec4(position,1.0);\n"
                                   "}";
    std::string fragmentShaderCode = "#extension GL_OES_EGL_image_external : require\n"
                                     "uniform vec4 color;\n"
                                     "varying vec2 v_Chord;\n"
                                     "uniform samplerExternalOES texture;\n"
                                     "void main()\n"
                                     "{\n"
                                     "gl_FragColor = texture2D(texture,v_Chord);\n"
                                     "}";

    GLCall(program = glCreateProgram())
    GLCall(Shader shader = Shader(program, vertexShaderCode, fragmentShaderCode))

    GLCall(glGenBuffers(2, arrayBuffer))
    GLCall(glBindBuffer(GL_ARRAY_BUFFER, arrayBuffer[0]))
    GLCall(glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_DYNAMIC_DRAW))

    GLCall(glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, arrayBuffer[1]))
    GLCall(glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices) * sizeof(unsigned int), indices,
                        GL_DYNAMIC_DRAW))

    GLCall(glLinkProgram(program))
    GLCall(glUseProgram(program))

    startCamera(jni, surface);
}

void Java_com_demo_opengl_CameraActivity_00024GL_00024Render_onSurfaceChanged(JNIEnv *jni,
                                                                              jobject object,
                                                                              jint width,
                                                                              jint height) {
    windowWidth = width;
    windowHeight = height;
}
void
Java_com_demo_opengl_CameraActivity_00024GL_00024Render_onDrawFrame(JNIEnv *jni, jobject object,
                                                                    jfloatArray array) {
    GLCall(glViewport(0, 0, windowWidth, windowHeight))
    GLCall(glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT))
    GLCall(glClearColor(0, 0, 0, 1))

    float mvp[] = {
            1.0f, 0, 0, 0,
            0, 1.0f, 0, 0,
            0, 0, 1.0f, 0,
            0, 0, 0, 1.0f
    };

    glm::mat4 u_mvp = glm::ortho(0.0f, float(windowWidth), 0.0f, float(windowHeight), 0.1f, 1.0f);

    glm::mat4 projection = u_mvp;

    GLCall(glActiveTexture(GL_TEXTURE0))
    GLCall(glBindTexture(GL_TEXTURE_EXTERNAL_OES, textureID))
    GLCall(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR))
    GLCall(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR))
    GLCall(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE))
    GLCall(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE))

    GLCall(int mvpLocation = glGetUniformLocation(program, "u_MVP"))
    GLCall(glUniformMatrix4fv(mvpLocation, 1, false, glm::value_ptr(projection)))

    GLCall(int positionHandle = glGetAttribLocation(program, "position"))
    GLCall(glEnableVertexAttribArray(positionHandle))
    GLCall(glVertexAttribPointer(positionHandle, 3, GL_FLOAT, false, 5 * sizeof(float),
                                 (GLvoid *) nullptr))

    GLCall(int chordsHandle = glGetAttribLocation(program, "texChords"))
    GLCall(glEnableVertexAttribArray(chordsHandle))
    GLCall(glVertexAttribPointer(chordsHandle, 2, GL_FLOAT, false, 5 * sizeof(float),
                                 (GLvoid *) (3 * sizeof(float))))

    // Pass SurfaceTexture transformations to shader
    float *tm = jni->GetFloatArrayElements(array, nullptr);
    GLCall(int matrixLocation = glGetUniformLocation(program, "texMatrix"))
    GLCall(glUniformMatrix4fv(matrixLocation, 1, false, tm))
    jni->ReleaseFloatArrayElements(array, tm, 0);

    GLCall(glBindBuffer(GL_ARRAY_BUFFER, arrayBuffer[0]))
    GLCall(glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, arrayBuffer[1]))

    GLCall(int textureLocation = glGetUniformLocation(program, "texture"))
    GLCall(glUniform1iv(textureLocation, sizeof(textureID), &textureID))

    GLCall(glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, nullptr))
}
}