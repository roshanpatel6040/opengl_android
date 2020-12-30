//
// Created by Roshan
//

#include <cstdio>
#include <unistd.h>
#include <jni.h>
#include <EGL/egl.h>
#include <cstdlib>
#include <dirent.h>
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
#include "GLES3/gl32.h"
#include "GLES3/gl3platform.h"
#include "GLES3/gl3.h"
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

int cameraWidth = 0;
int cameraHeight = 0;

int textureID;
GLuint program;
GLuint triangleProgram;
GLuint arrayBuffer[2];
GLuint positionHandle;
GLuint mvpLocation;
GLuint matrixLocation;
GLuint textureLocation;
GLuint chordsHandle;
GLuint saturationLocation;
GLuint contrastLocation;
GLuint brightnessLocation;

ACameraManager *cameraManager;
ACameraDevice *cameraDevice;
ACaptureRequest *previewRequest;
ACaptureRequest *captureRequest;
ACameraCaptureSession *session;
ANativeWindow *window;
ANativeWindow *imageWindow;
AImageReader *imageReader;
ACaptureSessionOutput *captureSessionOutput;
ACaptureSessionOutput *imageOutput;
ACaptureSessionOutputContainer *captureSessionOutputContainer;
ACaptureSessionOutputContainer *captureImageSessionOutputContainer;
ACameraOutputTarget *outputTarget;
ACameraOutputTarget *imageTarget;
ACameraMetadata *cameraMetadata;
ACameraMetadata_const_entry entry = {0};

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

static void onFailed(void *context, ACameraCaptureSession *session,
                     ACaptureRequest *request, ACameraCaptureFailure *failure) {
    __android_log_print(ANDROID_LOG_ERROR, "Capture", "failure %i", failure->reason);
}

static void onCaptureStarted(void *context, ACameraCaptureSession *session,
                             const ACaptureRequest *request, int64_t timestamp) {
    __android_log_print(ANDROID_LOG_DEBUG, "Capture", "time %lli", timestamp);
}

static void onCaptureCompleted(void *context, ACameraCaptureSession *session,
                               ACaptureRequest *request, const ACameraMetadata *result) {
    __android_log_print(ANDROID_LOG_DEBUG, "Capture", "%s", "Completed");
}

static ACameraCaptureSession_captureCallbacks captureCallbacks = {
        .context = nullptr,
        .onCaptureFailed = onFailed,
        .onCaptureStarted = onCaptureStarted,
        .onCaptureCompleted = onCaptureCompleted,
};

static void imageCallback(void *context, AImageReader *reader) {
    AImage *image = nullptr;
    auto status = AImageReader_acquireNextImage(reader, &image);
    if (status != AMEDIA_OK) {
        __android_log_print(ANDROID_LOG_ERROR, "Camera imageCallback", "%d", status);
    }
    std::thread processor([=]() {

        uint8_t *data = nullptr;
        int len = 0;
        AImage_getPlaneData(image, 0, &data, &len);

        const char *dirName = "/storage/emulated/0/pro/";
        DIR *dir = opendir(dirName);
        if (dir) {
            closedir(dir);
        } else {
            std::string command = "mkdir -p ";
            command += dirName;
            system(command.c_str());
        }

        std::string fileName = dirName;
        fileName += "preview.jpg";
        FILE *imageFile = std::fopen(fileName.c_str(), "wb");
        fwrite(data, 1, len, imageFile);
        fclose(imageFile);
        __android_log_print(ANDROID_LOG_DEBUG, "Camera", "%s", "Image saved");
        AImage_delete(image);
    });
    processor.detach();
}

AImageReader *createJpegReader() {
    AImageReader *reader = nullptr;
    media_status_t status = AImageReader_new(1920, 1080, AIMAGE_FORMAT_JPEG,
                                             4, &reader);

    if (status != AMEDIA_OK) {
        __android_log_print(ANDROID_LOG_ERROR, "Camera Reader", "%d", status);
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

void drawTriangle() {
    int CHORDS_COLOR_PER_VERTEX = 4;
    int BYTES_PER_FLOAT = 4;
    float vertices[] = {
            //    X    Y     Z     R     G      B    A
            0.0f, 0.5f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f,
            -0.5f, -0.5f, 0.0f, 1.0f, 0.0f, 0.0f, 1.0f,
            0.5f, -0.5f, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f};

    float color[] = {1.0f, 0.5f, 0.0f, 1.0f};

    // varying field uses to transfer data between vertex and fragment shader
    std::string vertexShaderCode = "attribute vec4 position;\n"
                                   "attribute vec4 v_color;\n"
                                   "varying vec4 f_color;\n" // Passes to fragment shader
                                   "void main()\n"
                                   "{\n"
                                   " f_color = v_color;\n"
                                   " gl_Position = position;\n"
                                   "}";
    std::string fragmentShaderCode = "uniform vec4 color;\n" // Get using uniform location
                                     "varying vec4 f_color;\n" // Get from vertex shader
                                     "void main()\n"
                                     "{\n"
                                     " gl_FragColor = f_color;\n"
                                     //                                " gl_FragColor = color;\n"
                                     "}";

    GLCall(triangleProgram = glCreateProgram())
    GLCall(Shader shader(triangleProgram, vertexShaderCode, fragmentShaderCode))
    GLCall(glLinkProgram(triangleProgram))
    GLCall(glUseProgram(triangleProgram))

    GLCall(VertexBuffer vb(vertices, sizeof(vertices)))

    GLCall(GLint positionHandle = shader.getAttributeLocation("position"))
    GLCall(shader.vertexAttribPointer(positionHandle, GL_FLOAT, 3, 7 * sizeof(float),
                                      (GLvoid *) nullptr))
    GLCall(shader.enableVertexAttribArray(positionHandle))

    GLCall(GLuint colorPositionHandle = shader.getAttributeLocation("v_color"))
    GLCall(shader.vertexAttribPointer(colorPositionHandle, GL_FLOAT, 4, 7 * sizeof(float),
                                      (GLvoid *) (3 * sizeof(float))))
    GLCall(shader.enableVertexAttribArray(colorPositionHandle))

    GLCall(int colorHandle = shader.getUniformLocation("color"))
    GLCall(shader.setUniform4fv(colorHandle,
                                sizeof(color) / CHORDS_COLOR_PER_VERTEX / BYTES_PER_FLOAT, color))

    GLCall(glDrawArrays(GL_TRIANGLES, 0, 3))
    GLCall(shader.disableVertexAttribPointer(positionHandle))
    GLCall(shader.disableVertexAttribPointer(colorPositionHandle))
    GLCall(shader.unBind())
    GLCall(vb.unBind())
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

void cameraDetails(std::string cameraId, JNIEnv *env, jobject object) {
    camera_status_t cameraCharacteristicStatus = ACameraManager_getCameraCharacteristics(
            cameraManager, cameraId.c_str(),
            &cameraMetadata);
    Camera(cameraCharacteristicStatus)

    // Exposure range
    ACameraMetadata_getConstEntry(cameraMetadata,
                                  ACAMERA_SENSOR_INFO_EXPOSURE_TIME_RANGE, &entry);
    int64_t minExposure = entry.data.i64[0];
    int64_t maxExposure = entry.data.i64[1];
    __android_log_print(ANDROID_LOG_DEBUG, "Camera exposure range", "%lli to %lli", minExposure,
                        maxExposure);

    jclass clazz = (*env).FindClass("com/demo/opengl/CameraActivity");
    jmethodID methodId = (*env).GetMethodID(clazz, "addExposureTime", "(JJ)V");
    (*env).CallVoidMethod(object, methodId, minExposure, maxExposure);

    // Sensitivity range
    ACameraMetadata_getConstEntry(cameraMetadata,
                                  ACAMERA_SENSOR_INFO_SENSITIVITY_RANGE, &entry);

    int32_t minSensitivity = entry.data.i32[0];
    int32_t maxSensitivity = entry.data.i32[1];
    __android_log_print(ANDROID_LOG_DEBUG, "Camera sensitivity", "%i to %i", minSensitivity,
                        maxSensitivity);

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
            __android_log_print(ANDROID_LOG_DEBUG, "Camera size", "width: %i, height: %i", width,
                                height);
        }
    }

    // Orientation
    ACameraMetadata_getConstEntry(cameraMetadata,
                                  ACAMERA_SENSOR_ORIENTATION, &entry);
    int32_t orientation = entry.data.i32[0];
    __android_log_print(ANDROID_LOG_DEBUG, "Camera orientation", "%i", orientation);

    // Frame duration
    ACameraMetadata_getConstEntry(cameraMetadata, ACAMERA_SENSOR_INFO_MAX_FRAME_DURATION, &entry);
    int64_t frameDuration = entry.data.i64[2];
    __android_log_print(ANDROID_LOG_DEBUG, "Camera frame duration", "%lli", frameDuration);
}

void createEglContext(JNIEnv *env, jobject surface) {
    static EGLint const attribute_list[] =
            {
                    EGL_RENDERABLE_TYPE, EGL_OPENGL_ES2_BIT,
                    EGL_RED_SIZE, 8,
                    EGL_GREEN_SIZE, 8,
                    EGL_BLUE_SIZE, 8,
                    EGL_DEPTH_SIZE, 16,
                    EGL_STENCIL_SIZE, 8,
                    EGL_NONE};

    static EGLint const context_list[] =
            {EGL_CONTEXT_CLIENT_VERSION, 2, EGL_NONE};

    EGLDisplay eglDisplay;
    EGLConfig eglConfig;
    EGLContext eglContext;
    EGLSurface eglSurface;
    NativeWindowType windowType;
    EGLint numConfig;

    EGLBoolean bind = eglBindAPI(EGL_OPENGL_ES_API);
    __android_log_print(ANDROID_LOG_ERROR, "Egl Bind", "%u", bind);
    // Get egl display connection
    EGLCall(eglDisplay = eglGetDisplay(EGL_DEFAULT_DISPLAY))
    // Initialize eglDisplay
    EGLCall(eglInitialize(eglDisplay, nullptr, nullptr))
    EGLCall(eglChooseConfig(eglDisplay, attribute_list, &eglConfig, 1, &numConfig))
    EGLCall(eglContext = eglCreateContext(eglDisplay, eglConfig, EGL_NO_CONTEXT, context_list))
    if (eglContext == nullptr) {
        __android_log_print(ANDROID_LOG_ERROR, "Egl error", "%s", "No context");
    }
    window = ANativeWindow_fromSurface(env, surface);
    EGLCall(eglSurface = eglCreateWindowSurface(eglDisplay, eglConfig, window, nullptr))
    if (eglSurface == EGL_NO_SURFACE) {
        __android_log_print(ANDROID_LOG_ERROR, "Egl error", "%s", "Unable to create surface");
        int error = eglGetError();
        switch (error) {
            case EGL_BAD_CONFIG:
                __android_log_print(ANDROID_LOG_ERROR, "Egl error", "%s", "Bad config");
                break;
            case EGL_BAD_NATIVE_WINDOW:
                __android_log_print(ANDROID_LOG_ERROR, "Egl error", "%s", "Bad window");
                break;
            case EGL_BAD_ALLOC:
                __android_log_print(ANDROID_LOG_ERROR, "Egl error", "%s", "Bad allocation");
                break;
        }
        return;
    }
    EGLCall(eglMakeCurrent(eglDisplay, eglSurface, eglSurface, eglContext))

    GLCall(glClearColor(1.0, 1.0, 0.0, 1.0))
    GLCall(glClear(GL_COLOR_BUFFER_BIT))
    GLCall(glFlush())

    EGLCall(eglSwapBuffers(eglDisplay, surface))
}

void openCamera(JNIEnv *env, jobject object) {
    string cameraID = getCameraId();
    // Open camera
    camera_status_t openCameraStatus = ACameraManager_openCamera(cameraManager,
                                                                 cameraID.c_str(),
                                                                 &stateCallbacks, &cameraDevice);
    Camera(openCameraStatus)

    cameraDetails(cameraID, env, object);
}

void captureImage() {
    camera_status_t captureRequestStatus = ACameraDevice_createCaptureRequest(cameraDevice,
                                                                              TEMPLATE_STILL_CAPTURE,
                                                                              &captureRequest);
    Camera(captureRequestStatus)

    ANativeWindow_acquire(window);

    camera_status_t addTargetStatus = ACaptureRequest_addTarget(captureRequest, imageTarget);
    Camera(addTargetStatus)

    camera_status_t captureStatus = ACameraCaptureSession_capture(session, &captureCallbacks, 1,
                                                                  &captureRequest, nullptr);
    Camera(captureStatus)
}

void startCamera(JNIEnv *env, jobject object) {
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
    imageReader = createJpegReader();
    imageWindow = createSurface(imageReader);
    ACameraOutputTarget_create(imageWindow, &imageTarget);
    ACaptureSessionOutput_create(imageWindow, &imageOutput);
    ACaptureSessionOutputContainer_add(captureSessionOutputContainer, imageOutput);

    ANativeWindow_acquire(window);
    camera_status_t outputTargetStatus = ACameraOutputTarget_create(window, &outputTarget);
    Camera(outputTargetStatus)
    camera_status_t addTargetStatus = ACaptureRequest_addTarget(previewRequest, outputTarget);
    Camera(addTargetStatus)

    uint8_t controlMode = ACAMERA_CONTROL_MODE_AUTO;
    camera_status_t controlModeStatus = ACaptureRequest_setEntry_u8(previewRequest,
                                                                    ACAMERA_CONTROL_MODE,
                                                                    1,
                                                                    &controlMode);
    Camera(controlModeStatus)

    uint8_t controlAfMode = ACAMERA_CONTROL_AF_MODE_CONTINUOUS_PICTURE;
    camera_status_t controlModeAFStatus = ACaptureRequest_setEntry_u8(previewRequest,
                                                                      ACAMERA_CONTROL_AF_MODE,
                                                                      1,
                                                                      &controlAfMode);
    Camera(controlModeAFStatus)

    uint8_t controlAWBMode = ACAMERA_CONTROL_AWB_MODE_AUTO;
    camera_status_t controlModeAWBStatus = ACaptureRequest_setEntry_u8(previewRequest,
                                                                       ACAMERA_CONTROL_AWB_MODE,
                                                                       1,
                                                                       &controlAWBMode);
    Camera(controlModeAWBStatus)

//    const uint8_t correctionMode = ACAMERA_COLOR_CORRECTION_MODE_TRANSFORM_MATRIX;
//    camera_status_t colorCorrectStatus = ACaptureRequest_setEntry_u8(previewRequest,
//                                                                     ACAMERA_COLOR_CORRECTION_MODE,
//                                                                     1, &correctionMode);
//    Camera(colorCorrectStatus)

//    const float gainValues[] = {3.0,0.0,0.0,0.0};
//    camera_status_t colorGainStatus = ACaptureRequest_setEntry_float(previewRequest,
//                                                                     ACAMERA_COLOR_CORRECTION_GAINS,
//                                                                     1,
//                                                                     reinterpret_cast<const float *>(&gainValues));
//    Camera(colorGainStatus)

    // Turn off AE to apply sensor sensitivity and exposure time
    uint8_t controlAEMode = ACAMERA_CONTROL_AE_MODE_ON;
    camera_status_t controlModeAEStatus = ACaptureRequest_setEntry_u8(previewRequest,
                                                                      ACAMERA_CONTROL_AE_MODE,
                                                                      1,
                                                                      &controlAEMode);
    Camera(controlModeAEStatus)
    int64_t sensorExposureTime = 500000;
    camera_status_t expTimeStatus = ACaptureRequest_setEntry_i64(previewRequest,
                                                                 ACAMERA_SENSOR_EXPOSURE_TIME, 1,
                                                                 &sensorExposureTime);
    Camera(expTimeStatus)

    int32_t orientation = 90;
    camera_status_t jpegOrientationStatus = ACaptureRequest_setEntry_i32(previewRequest,
                                                                         ACAMERA_JPEG_ORIENTATION,
                                                                         1,
                                                                         &orientation);
    Camera(jpegOrientationStatus)

    const int32_t sensor = 800;
    camera_status_t sensorStatus = ACaptureRequest_setEntry_i32(previewRequest,
                                                                ACAMERA_SENSOR_SENSITIVITY, 1,
                                                                &sensor);
    Camera(sensorStatus)

//    const int64_t frameDuration = 3000000;
//    camera_status_t sensorFrameDurationStatus = ACaptureRequest_setEntry_i64(previewRequest,
//                                                                             ACAMERA_SENSOR_FRAME_DURATION,
//                                                                             1,
//                                                                             &frameDuration);
//    Camera(sensorFrameDurationStatus)

    float lensAperture = 1.5;
    camera_status_t lensApertureStatus = ACaptureRequest_setEntry_float(previewRequest,
                                                                        ACAMERA_LENS_APERTURE, 1,
                                                                        &lensAperture);
    Camera(lensApertureStatus)

//    int64_t frameDurationTime = 30000000;
//    camera_status_t frameDurationStatus = ACaptureRequest_setEntry_i64(previewRequest,
//                                                                 ACAMERA_SENSOR_FRAME_DURATION, 1,
//                                                                 &frameDurationTime);
//    Camera(frameDurationStatus)

//    const uint8_t noiseMode = ACAMERA_NOISE_REDUCTION_MODE_MINIMAL;
//    camera_status_t noiseReductionStatus = ACaptureRequest_setEntry_u8(previewRequest,
//                                                                       ACAMERA_NOISE_REDUCTION_MODE,
//                                                                       1, &noiseMode);
//    Camera(noiseReductionStatus)

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

void closeCamera() {
    ACameraMetadata_free(cameraMetadata);
    ACaptureRequest_free(previewRequest);
    ACameraDevice_close(cameraDevice);
    ACameraCaptureSession_close(session);
    ACameraManager_delete(cameraManager);
}

extern "C" {
void Java_com_demo_opengl_CameraActivity_initialize(JNIEnv *jni, jobject object) {
    cameraManager = ACameraManager_create();
    openCamera(jni, object);
}

void
Java_com_demo_opengl_CameraActivity_changeExposure(JNIEnv *jni, jobject object, jint exposure) {
    int64_t sensorExposureTime = exposure;
    camera_status_t expTimeStatus = ACaptureRequest_setEntry_i64(previewRequest,
                                                                 ACAMERA_SENSOR_EXPOSURE_TIME, 1,
                                                                 &sensorExposureTime);
    Camera(expTimeStatus)
    // set repeating request
    camera_status_t repeatingRequestStatus = ACameraCaptureSession_setRepeatingRequest(session,
                                                                                       nullptr, 1,
                                                                                       &previewRequest,
                                                                                       nullptr);
}

void Java_com_demo_opengl_CameraActivity_destroy(JNIEnv *jni, jobject object) {
    closeCamera();
}

void Java_com_demo_opengl_CameraActivity_00024GL_capture(JNIEnv *jni, jobject object) {
    captureImage();
}

void Java_com_demo_opengl_CameraActivity_00024GL_00024Render_onSurfaceCreated(JNIEnv *jni,
                                                                              jobject object,
                                                                              jint textureId,
                                                                              jobject surface,
                                                                              jint width,
                                                                              jint height) {
    textureID = textureId;
    cameraWidth = width;
    cameraHeight = height;

    // Create window with surface
    window = ANativeWindow_fromSurface(jni, surface);
//    createEglContext(jni, surface);

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
                                   "v_Chord = (texMatrix * vec4(texChords.x, texChords.y, 0.0, 1.0)).xy;\n"
                                   "gl_Position = u_MVP * vec4(position,1.0);\n"
                                   "}";
    std::string fragmentShaderCode = "#extension GL_OES_EGL_image_external : require\n"
                                     "precision highp float;\n"
                                     "uniform highp vec4 color;\n"
                                     "varying lowp vec2 v_Chord;\n"
                                     "uniform samplerExternalOES texture;\n"
                                     "uniform float u_saturation;\n"
                                     "uniform float u_contrast;\n"
                                     "uniform float u_brightness;\n"
                                     "const float Epsilon = 1e-10;\n"
                                     "\n"
                                     "// ***** Rgb to hsv ***** \n"
                                     "vec3 RGBtoHSV(in vec3 RGB)\n"
                                     " {\n"
                                     "        vec4  P   = (RGB.g < RGB.b) ? vec4(RGB.bg, -1.0, 2.0/3.0) : vec4(RGB.gb, 0.0, -1.0/3.0);\n"
                                     "        vec4  Q   = (RGB.r < P.x) ? vec4(P.xyw, RGB.r) : vec4(RGB.r, P.yzx);\n"
                                     "        float C   = Q.x - min(Q.w, Q.y);\n"
                                     "        float H   = abs((Q.w - Q.y) / (6.0 * C + Epsilon) + Q.z);\n"
                                     "        vec3  HCV = vec3(H, C, Q.x);\n"
                                     "        float S   = HCV.y / (HCV.z + Epsilon);\n"
                                     "        return vec3(HCV.x, S, HCV.z);\n"
                                     " }\n"
                                     "// ***** Hsv to rgb ***** \n"
                                     "\n"
                                     " vec3 HSVtoRGB(in vec3 HSV)\n"
                                     " {\n"
                                     "        float H   = HSV.x;\n"
                                     "        float R   = abs(H * 6.0 - 3.0) - 1.0;\n"
                                     "        float G   = 2.0 - abs(H * 6.0 - 2.0);\n"
                                     "        float B   = 2.0 - abs(H * 6.0 - 4.0);\n"
                                     "        vec3  RGB = clamp( vec3(R,G,B), 0.0, 1.0 );\n"
                                     "        return ((RGB - 1.0) * HSV.y + 1.0) * HSV.z;\n"
                                     "}\n"
                                     "\n"
                                     "// ***** Brightness ***** \n"
                                     "vec4 brightness(vec4 color,float brightness)\n"
                                     "{\n"
                                     "        vec4 transformedColor = vec4(color.rgb + brightness,1.0);\n"
                                     "        return clamp(transformedColor,0.0,1.0);\n"
                                     "}\n"
                                     "// ***** Contrast ***** \n"
                                     "vec4 contrast(vec4 color,float contrast)\n"
                                     "{\n"
                                     "        vec4 contrastColor = vec4(((color.rgb-vec3(0.5))*contrast)+vec3(0.5), 1.0);\n"
                                     "        return clamp(contrastColor,0.0,1.0);\n"
                                     "}\n"
                                     "\n"
                                     "\n"
                                     "void main()\n"
                                     "{\n"
                                     "vec4 frag = texture2D(texture,v_Chord);\n"
                                     "vec3 color = frag.xyz;\n"
                                     "vec3 col_hsv = RGBtoHSV(color.rgb);\n"
                                     "col_hsv.y *= (u_saturation * 2.0); \n"
                                     "vec3 col_rgb = HSVtoRGB(col_hsv.rgb);\n"
                                     "vec4 final = vec4(col_rgb.rgb,1.0);\n"
                                     "vec4 contrast = contrast(final,u_contrast);\n"
                                     "gl_FragColor = brightness(contrast,u_brightness);\n"
                                     "}";
    __android_log_print(ANDROID_LOG_ERROR, "OpenGL version", "%s", glGetString(GL_VERSION));
    __android_log_print(ANDROID_LOG_ERROR, "OpenGL shader version", "%s",
                        glGetString(GL_SHADING_LANGUAGE_VERSION));
    GLCall(program = glCreateProgram())
    GLCall(Shader shader = Shader(program, vertexShaderCode, fragmentShaderCode))

    GLCall(glBindAttribLocation(program, 0, "position"))

    GLCall(glLinkProgram(program))

    GLboolean isProgram = glIsProgram(program);
    if (isProgram == GL_FALSE) {
        __android_log_print(ANDROID_LOG_ERROR, "OpenGL isProgram", "%s", "Failed");
    }
    GLint status;
    glGetProgramiv(program, GL_LINK_STATUS, &status);
    if (status == GL_FALSE) {
        __android_log_print(ANDROID_LOG_ERROR, "OpenGL program status", "%s", "Failed");
        GLsizei logLength;
        GLchar log[1024];
        glGetProgramInfoLog(program, sizeof(log), &logLength, log);
        __android_log_print(ANDROID_LOG_ERROR, "OpenGL program status", "%s", log);
    }

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
    GLCall(saturationLocation = glGetUniformLocation(program, "u_saturation"))
    GLCall(contrastLocation = glGetUniformLocation(program, "u_contrast"))
    GLCall(brightnessLocation = glGetUniformLocation(program, "u_brightness"))

    startCamera(jni, object);
}

void Java_com_demo_opengl_CameraActivity_00024GL_00024Render_onSurfaceChanged(JNIEnv *jni,
                                                                              jobject object,
                                                                              jint width,
                                                                              jint height) {
    windowWidth = width;
    windowHeight = height;
    ANativeWindow_acquire(window);
    ANativeWindow_setBuffersGeometry(window, windowWidth, windowHeight, WINDOW_FORMAT_RGBA_8888);

}
void
Java_com_demo_opengl_CameraActivity_00024GL_00024Render_onDrawFrame(JNIEnv *jni, jobject object,
                                                                    jfloatArray array,
                                                                    jfloat saturation,
                                                                    jfloat contrast,
                                                                    jfloat brightness) {
    GLCall(glViewport(0, 0, windowWidth, windowHeight))
    GLCall(glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT))
    GLCall(glClearColor(0, 0, 0, 1))

    GLCall(glUseProgram(program))

    float mvp[] = {
            1.0f, 0.0f, 0.0f, 0.0f,
            0.0f, 1.0f, 0.0f, 0.0f,
            0.0f, 0.0f, 1.0f, 0.0f,
            0.0f, 0.0f, 0.0f, 1.0f,
    };

    float aspect = windowWidth > windowHeight ? float(windowWidth) / float(windowHeight) :
                   float(windowHeight) / float(windowWidth);
    if (windowWidth < windowHeight) // portrait
        ortho(mvp, -0.823333f, 0.823333f, -1.0, 1.0, -1.0f, 1.0f);
    else // landscape
        ortho(mvp, -aspect, aspect, -1.0f, 1.0f, -1.0f, 1.0f);

    GLCall(glUniformMatrix4fv(mvpLocation, 1, false, mvp))

    GLCall(glActiveTexture(GL_TEXTURE0))
    GLCall(glBindTexture(GL_TEXTURE_EXTERNAL_OES, textureID))
    GLCall(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR))
    GLCall(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR))
    GLCall(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE))
    GLCall(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE))

    // Pass SurfaceTexture transformations to shader
    float *tm = jni->GetFloatArrayElements(array, nullptr);
    GLCall(glUniformMatrix4fv(matrixLocation, 1, false, tm))
    jni->ReleaseFloatArrayElements(array, tm, 0);

    GLCall(glUniform1i(textureLocation, 0))

    GLCall(glUniform1f(saturationLocation, saturation))
    GLCall(glUniform1f(contrastLocation, contrast))
    GLCall(glUniform1f(brightnessLocation, brightness))

    GLCall(glBindBuffer(GL_ARRAY_BUFFER, arrayBuffer[0]))
    GLCall(glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, arrayBuffer[1]))

    GLCall(glEnableVertexAttribArray(positionHandle))
    GLCall(glVertexAttribPointer(positionHandle, 3, GL_FLOAT, false, 5 * sizeof(float),
                                 (GLvoid *) nullptr))
    GLCall(glEnableVertexAttribArray(chordsHandle))
    GLCall(glVertexAttribPointer(chordsHandle, 2, GL_FLOAT, false, 5 * sizeof(float),
                                 (GLvoid *) (3 * sizeof(float))))

    GLCall(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE))
    GLCall(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE))

    GLCall(glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, nullptr))

    GLCall(glDisableVertexAttribArray(positionHandle))
    GLCall(glDisableVertexAttribArray(chordsHandle))
}
}