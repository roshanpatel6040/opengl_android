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

int windowWidth = 640;
int windowHeight = 480;

int textureID;
GLuint program;
GLuint arrayBuffer[2];
GLuint positionHandle;
GLuint mvpLocation;
GLuint matrixLocation;
GLuint textureLocation;
GLuint chordsHandle;

ACameraManager *cameraManager;
ACameraDevice *cameraDevice;
ACaptureRequest *previewRequest;
ACameraCaptureSession *session;
ANativeWindow *window;
ANativeWindow *imageWindow;
AImageReader *imageReader;
ACaptureSessionOutput *captureSessionOutput;
ACaptureSessionOutput *imageOutput;
ACaptureSessionOutputContainer *captureSessionOutputContainer;
ACameraOutputTarget *outputTarget;
ACameraOutputTarget *imageTarget;
ACameraMetadata *cameraMetadata;

#define Camera(x) \
    if(x != ACAMERA_OK){ \
        __android_log_print(ANDROID_LOG_ERROR,"Camera status","Line: %i, error: %i",__LINE__,x); \
    } \


void ortho(float *mat4, float left, float right, float bottom, float top, float near, float far) {
    float rl = right - left;
    float tb = top - bottom;
    float fn = far - near;

    mat4[0] = 2.0f / rl;
    mat4[12] = -(right + left) / rl;

    mat4[5] = 2.0f / tb;
    mat4[13] = -(top + bottom) / tb;

    mat4[10] = -2.0f / fn;
    mat4[14] = -(far + near) / fn;
}

static void onDisconnected(void *context, ACameraDevice *device) {
    __android_log_print(ANDROID_LOG_ERROR, "Camera", "%s", "Disconnected");
}

static void onError(void *context, ACameraDevice *device, int error) {
    __android_log_print(ANDROID_LOG_ERROR, "Camera", "%i", error);
}

static void onActive(void *context, ACameraCaptureSession *session) {
    __android_log_print(ANDROID_LOG_ERROR, "Camera session", "%s", "active");
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
        __android_log_print(ANDROID_LOG_DEBUG, "Camera", "%s", "Captured");

        AImage_delete(image);
    });
    processor.detach();
}

AImageReader *createJpegReader() {
    AImageReader *reader = nullptr;
    media_status_t status = AImageReader_new(640, 480, AIMAGE_FORMAT_JPEG,
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

string getCameraId() {
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
    return backFacingId;
}

void cameraDetails(std::string cameraId) {
    camera_status_t cameraCharacteristicStatus = ACameraManager_getCameraCharacteristics(
            cameraManager, cameraId.c_str(),
            &cameraMetadata);
    Camera(cameraCharacteristicStatus)

    ACameraMetadata_const_entry entry = {0};
    ACameraMetadata_getConstEntry(cameraMetadata,
                                  ACAMERA_SENSOR_INFO_EXPOSURE_TIME_RANGE, &entry);

    int64_t minExposure = entry.data.i64[0];
    int64_t maxExposure = entry.data.i64[1];

    // sensitivity
    ACameraMetadata_getConstEntry(cameraMetadata,
                                  ACAMERA_SENSOR_INFO_SENSITIVITY_RANGE, &entry);

    int32_t minSensitivity = entry.data.i32[0];
    int32_t maxSensitivity = entry.data.i32[1];

    // JPEG format
    ACameraMetadata_getConstEntry(cameraMetadata,
                                  ACAMERA_SCALER_AVAILABLE_STREAM_CONFIGURATIONS, &entry);

    for (int i = 0; i < entry.count; i += 4) {
        // We are only interested in output streams, so skip input stream
        int32_t input = entry.data.i32[i + 3];
        if (input)
            continue;

        int32_t format = entry.data.i32[i + 0];
        if (format == AIMAGE_FORMAT_JPEG) {
            int32_t width = entry.data.i32[i + 1];
            int32_t height = entry.data.i32[i + 2];
        }
    }

    ACameraMetadata_getConstEntry(cameraMetadata,
                                  ACAMERA_SENSOR_ORIENTATION, &entry);

    int32_t orientation = entry.data.i32[0];
}

void startCamera() {
    string cameraID = getCameraId();
    // Open camera
    camera_status_t openCameraStatus = ACameraManager_openCamera(cameraManager,
                                                                 cameraID.c_str(),
                                                                 &stateCallbacks, &cameraDevice);
    Camera(openCameraStatus)

    cameraDetails(cameraID);

    // Create capture request
    camera_status_t captureRequestStatus = ACameraDevice_createCaptureRequest(cameraDevice,
                                                                              TEMPLATE_PREVIEW,
                                                                              &previewRequest);
    Camera(captureRequestStatus)


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


    // Image reader
//    imageReader = createJpegReader();
//    imageWindow = createSurface(imageReader);
//    ANativeWindow_acquire(imageWindow);
//    ACameraOutputTarget_create(imageWindow, &imageTarget);
//    ACaptureRequest_addTarget(previewRequest, imageTarget);
//    ACaptureSessionOutput_create(imageWindow, &imageOutput);
//    ACaptureSessionOutputContainer_add(captureSessionOutputContainer, imageOutput);

    // Acquire window
    ANativeWindow_acquire(window);
    camera_status_t outputTargetStatus = ACameraOutputTarget_create(window, &outputTarget);
    Camera(outputTargetStatus)
    camera_status_t addTargetStatus = ACaptureRequest_addTarget(previewRequest, outputTarget);
    Camera(addTargetStatus)

    // Create capture session
    camera_status_t captureSessionStatus = ACameraDevice_createCaptureSession(cameraDevice,
                                                                              captureSessionOutputContainer,
                                                                              &sessionCallbacks,
                                                                              &session);
    Camera(captureSessionStatus)
    // set repeating request
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
            1.0f, 1.0f, 0.0f, 1.0f, 1.0f,
            1.0f, -1.0f, 0.0f, 1.0f, 0.0f
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
                                   "gl_Position = u_MVP * vec4(position,1.0f);\n"
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
    GLCall(glLinkProgram(program))

    GLCall(glGenBuffers(2, arrayBuffer))
    GLCall(glBindBuffer(GL_ARRAY_BUFFER, arrayBuffer[0]))
    GLCall(glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_DYNAMIC_DRAW))

    GLCall(glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, arrayBuffer[1]))
    GLCall(glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices,
                        GL_DYNAMIC_DRAW))

    GLCall(positionHandle = glGetAttribLocation(program, "position"))
    GLCall(chordsHandle = glGetAttribLocation(program, "texChords"))
    GLCall(mvpLocation = glGetUniformLocation(program, "u_MVP"))
    GLCall(matrixLocation = glGetUniformLocation(program, "texMatrix"))
    GLCall(textureLocation = glGetUniformLocation(program, "texture"))


    // Create window with surface
    window = ANativeWindow_fromSurface(jni, surface);

    startCamera();
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
    GLCall(glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT))
    GLCall(glClearColor(0, 0, 0, 1))

    GLCall(glUseProgram(program))

    float mvp[] = {
            1.0f, 0, 0, 0,
            0, 1.0f, 0, 0,
            0, 0, 1.0f, 0,
            0, 0, 0, 1.0f
    };

    float aspect = windowWidth > windowHeight ? float(windowWidth) / float(windowHeight) :
                   float(windowHeight) / float(windowWidth);
    if (windowWidth < windowHeight) // portrait
        ortho(mvp, -1.0f, 1.0f, -aspect, aspect, -1.0f, 1.0f);
    else // landscape
        ortho(mvp, -aspect, aspect, -1.0f, 1.0f, -1.0f, 1.0f);

    GLCall(glUniformMatrix4fv(mvpLocation, 1, false, mvp))

    GLCall(glActiveTexture(GL_TEXTURE0))
    GLCall(glBindTexture(GL_TEXTURE_EXTERNAL_OES, textureID))
//    GLCall(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR))
//    GLCall(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR))
    GLCall(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE))
    GLCall(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE))

    // Pass SurfaceTexture transformations to shader
    float *tm = jni->GetFloatArrayElements(array, nullptr);
    GLCall(glUniformMatrix4fv(matrixLocation, 1, false, tm))
    jni->ReleaseFloatArrayElements(array, tm, 0);

    GLCall(glUniform1i(textureLocation, 0))

    GLCall(glBindBuffer(GL_ARRAY_BUFFER, arrayBuffer[0]))
    GLCall(glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, arrayBuffer[1]))

    GLCall(glEnableVertexAttribArray(positionHandle))
    GLCall(glVertexAttribPointer(positionHandle, 3, GL_FLOAT, false, 5 * sizeof(float),
                                 (GLvoid *) nullptr))
    GLCall(glEnableVertexAttribArray(chordsHandle))
    GLCall(glVertexAttribPointer(chordsHandle, 2, GL_FLOAT, false, 5 * sizeof(float),
                                 (GLvoid *) (3 * sizeof(float))))

    GLCall(glViewport(0, 0, windowWidth, windowHeight))
    GLCall(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE))
    GLCall(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE))

    GLCall(glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, nullptr))

    GLCall(glDisableVertexAttribArray(positionHandle))
    GLCall(glDisableVertexAttribArray(chordsHandle))
}
}