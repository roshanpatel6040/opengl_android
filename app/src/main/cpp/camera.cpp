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
        }
    }

    ACameraMetadata_getConstEntry(cameraMetadata,
                                  ACAMERA_SENSOR_ORIENTATION, &entry);

    int32_t orientation = entry.data.i32[0];

//    const uint32_t **allTags;
//    int32_t count;
//    ACameraMetadata_getAllTags(cameraMetadata, &count, allTags);

//    for (int i = 0; i < sizeof(allTags); ++i) {
//        __android_log_print(ANDROID_LOG_DEBUG, "All tags", "%i", allTags[i]);
//    }
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

    uint8_t controlAfMode = ACAMERA_CONTROL_AF_MODE_AUTO;
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

    uint8_t controlAEMode = ACAMERA_CONTROL_AE_MODE_ON;
    camera_status_t controlModeAEStatus = ACaptureRequest_setEntry_u8(previewRequest,
                                                                      ACAMERA_CONTROL_AE_MODE,
                                                                      1,
                                                                      &controlAEMode);
    Camera(controlModeAEStatus)

    int32_t orientation = 90;
    camera_status_t jpegOrientationStatus = ACaptureRequest_setEntry_i32(previewRequest,
                                                                         ACAMERA_JPEG_ORIENTATION,
                                                                         1,
                                                                         &orientation);

    const int32_t sensor = 800;
    camera_status_t sensorStatus = ACaptureRequest_setEntry_i32(previewRequest,
                                                                ACAMERA_SENSOR_SENSITIVITY, 1,
                                                                &sensor);
    Camera(sensorStatus)

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
}

void Java_com_demo_opengl_CameraActivity_destroy(JNIEnv *jni, jobject object) {
    closeCamera();
}

extern "C" {
void Java_com_demo_opengl_CameraActivity_00024GL_capture(JNIEnv *jni, jobject object) {
    __android_log_print(ANDROID_LOG_DEBUG, "Capture", "%s", "Initiated");
    camera_status_t captureStatus = ACameraCaptureSession_capture(session, &captureCallbacks, 1,
                                                                  &previewRequest, nullptr);
    Camera(captureStatus)
}
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
                                   "gl_Position = u_MVP * vec4(position,1.0f);\n"
                                   "}";
    std::string fragmentShaderCode = "#extension GL_OES_EGL_image_external : require\n"
                                     "uniform vec4 color;\n"
                                     "varying vec2 v_Chord;\n"
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
                                     "vec4 frag = texture2D(texture,v_Chord,0.0f);\n"
                                     "vec3 color = frag.xyz;\n"
                                     "vec3 col_hsv = RGBtoHSV(color.rgb);\n"
                                     "col_hsv.y *= (u_saturation * 2.0); \n"
                                     "vec3 col_rgb = HSVtoRGB(col_hsv.rgb);\n"
                                     "vec4 final = vec4(col_rgb.rgb,1.0);\n"
                                     "vec4 contrast = contrast(final,u_contrast);\n"
                                     "gl_FragColor = brightness(contrast,u_brightness);\n"
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
    GLCall(saturationLocation = glGetUniformLocation(program, "u_saturation"))
    GLCall(contrastLocation = glGetUniformLocation(program, "u_contrast"))
    GLCall(brightnessLocation = glGetUniformLocation(program, "u_brightness"))


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