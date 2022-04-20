//
// Created by Roshan
//

#include <vector>
#include <cstdio>
#include <unistd.h>
#include <jni.h>
#include <cstdlib>
#include <dirent.h>
#include <map>

#include <EGL/egl.h>
#include <GLES/gl.h>
#include <GLES/glext.h>
#include <GLES/glplatform.h>
#include <GLES/egl.h>
#include <GLES2/gl2.h>
#include <GLES2/gl2platform.h>
#include <GLES2/gl2ext.h>
#include "GLES3/gl32.h"
#include "GLES3/gl3platform.h"
#include "GLES3/gl3.h"
#include "GLES3/gl31.h"
#include "GLES3/gl3ext.h"
#include "glm/glm.hpp"
#include "glm/ext.hpp"

#include <string>
#include <android/log.h>
#include <iostream>
#include <cassert>
#include "stb_image/stb_image.cpp"
#include <android/asset_manager_jni.h>
#include <android/asset_manager.h>
#include <CameraView.h>
#include <gtx/quaternion.hpp>
#include "core/headers/Renderer.h"
#include "core/headers/Shader.h"
#include "core/headers/Texture.h"
#include "core/headers/Mesh.h"
#include "core/headers/VertexBuffer.h"
#include "camera/NdkCameraCaptureSession.h"
#include "camera/NdkCameraDevice.h"
#include "camera/NdkCameraError.h"
#include "camera/NdkCameraManager.h"
#include "camera/NdkCameraMetadata.h"
#include "camera/NdkCameraMetadataTags.h"
#include "camera/NdkCameraWindowType.h"
#include "android/native_window.h"
#include "android/native_activity.h"
#include "android/native_window_jni.h"
#include "media/NdkImageReader.h"
#include "media/NdkImage.h"
#include "media/NdkMediaFormat.h"
#include "thread"

#include "android/sensor.h"
#include "assimp/Importer.hpp"
#include "assimp/scene.h"
#include <assimp/port/AndroidJNI/AndroidJNIIOSystem.h>

using namespace std;

int windowWidth = 640;
int windowHeight = 480;

int cameraWidth = 0;
int cameraHeight = 0;

int detectionWidth = 640;
int detectionHeight = 480;

int textureID;
GLuint program;
GLuint arrayBuffer[4];
GLuint positionHandle;
GLuint mvpLocation;
GLuint matrixLocation;
GLuint textureLocation;
GLuint chordsHandle;
GLuint saturationLocation;
GLuint contrastLocation;
GLuint brightnessLocation;
GLuint highLightLocation;
GLuint shadowLocation;
GLuint awbLocation;
GLuint textureLutReferenceID;
GLuint lutTextureLocation;
float applyAwb = 0;

ACameraManager *cameraManager;
ACameraDevice *cameraDevice;
ACaptureRequest *previewRequest;
ACaptureRequest *captureRequest;
ACameraCaptureSession *session;
ANativeWindow *previewWindow;
ANativeWindow *imageWindow;
AImageReader *imageReader;
ACaptureSessionOutput *previewSessionOutput;
ACaptureSessionOutput *captureSessionOutput;
ACaptureSessionOutputContainer *previewSessionOutputContainer;
ACaptureSessionOutputContainer *captureSessionOutputContainer;
ACameraOutputTarget *previewOutputTarget;
ACameraOutputTarget *captureOutputTarget;
ACameraMetadata *cameraMetadata;
ACameraMetadata_const_entry entry = {0};

ASensorManager *sensorManager;
AAssetManager *assetManager;

JavaVM *javaVM = nullptr;
jclass activityClass;
jobject activityObj;

GLuint meshProgram;
Shader *meshShader;
CameraView *meshCamera;
Mesh *mesh;

GLuint videoProgram;
Shader *videoShader;
VertexBuffer *videoVertexBuffer;
IndexBuffer *videoIndexBuffer;
GLuint videoTextureId;

const string vertexShaderCode = "#version 320 es\n"
                                "in vec3 position;\n"
                                "in vec2 texChords;\n"
                                "uniform mat4 texMatrix;\n"
                                "uniform mat4 u_MVP;\n"
                                "out vec2 v_Chord;\n"
                                "void main()\n"
                                "{\n"
                                "v_Chord = (texMatrix * vec4(texChords.x, texChords.y, 0.0, 1.0)).xy;\n"
                                "gl_Position = u_MVP * vec4(position,1.0);\n"
                                "}";
const string fragmentShaderCode = "#version 320 es\n"
                                  "#extension GL_OES_EGL_image_external_essl3 : require\n"
                                  "precision highp float;\n"
                                  "uniform highp vec4 color;\n"
                                  "in lowp vec2 v_Chord;\n"
                                  "uniform samplerExternalOES tex;\n"
                                  "uniform highp sampler2D textureLut;\n"
                                  "uniform float u_saturation;\n"
                                  "uniform float u_contrast;\n"
                                  "uniform float u_brightness;\n"
                                  "uniform float u_highlight;\n"
                                  "uniform float u_shadow;\n"
                                  "uniform float u_awb; // awb 0 off and 1 on\n"
                                  "const float Epsilon = 1e-10;\n"
                                  "const float cellsPerRow = 8.0;\n"
                                  "const float cellSize = 0.125;\n"
                                  "const float halfTexelSize = 0.0009765625;\n"
                                  "const float cellSizeFixed = 0.123046875;\n"
                                  "out vec4 fragColor;\n"
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
                                  "// ***** Lut Filter ***** \n"
                                  "vec4 lut(vec4 color)\n"
                                  "{\n"
                                  "float alpha = color.a;\n"
                                  "vec4 pmc = vec4(color.rgb/alpha,alpha);\n"
                                  "vec4 lookup = texture(textureLut,pmc.xy);\n"
                                  "lookup.a = 1.0;\n"
                                  "return lookup;\n"
                                  "}\n"
                                  "vec4 lutFilter(vec4 color)\n"
                                  "{\n"
                                  "float b = color.b * (cellsPerRow * cellsPerRow - 1.0);\n"
                                  "vec2 lc,ls,uc,us;\n"
                                  "lc.y = floor(b / cellsPerRow);\n"
                                  "lc.x = floor(b) - lc.y * cellsPerRow;\n"
                                  "ls.x = lc.x * cellSize + halfTexelSize + cellSizeFixed * color.r;\n"
                                  "ls.y = lc.y * cellSize + halfTexelSize + cellSizeFixed * color.g;\n"
                                  "uc.y = floor(ceil(b) / cellsPerRow);\n"
                                  "uc.x = ceil(b) - uc.y * cellsPerRow;\n"
                                  "us.x = uc.x * cellSize + halfTexelSize + cellSizeFixed * color.r;\n"
                                  "us.y = uc.y * cellSize + halfTexelSize + cellSizeFixed * color.g;\n"
                                  "vec3 outColor = mix(texture(textureLut,ls).rgb,texture(textureLut,us).rgb, fract(b));\n"
                                  "outColor = mix(color.rgb, outColor,0.15555);\n" // last parameter ranges 0.0 to 1.0 -> 1.0 show lut color
                                  "return vec4(outColor,color.x);\n"
                                  "}\n"
                                  "vec4 whiteBalance(vec4 source,float temp,float tint)\n"
                                  "{\n"
                                  "lowp vec3 warmFilter = vec3(0.93, 0.54, 0.0);\n"
                                  "mediump mat3 RGBtoYIQ = mat3(0.299, 0.587, 0.114, 0.596, -0.274, -0.322, 0.212, -0.523, 0.311);\n"
                                  "mediump mat3 YIQtoRGB = mat3(1.0, 0.956, 0.621, 1.0, -0.272, -0.647, 1.0, -1.105, 1.702);\n"
                                  "mediump vec3 yiq = RGBtoYIQ * source.rgb;\n"
                                  "yiq.b = clamp(yiq.b + tint*0.5226*0.1, -0.5226, 0.5226);\n"
                                  "lowp vec3 rgb = YIQtoRGB * yiq;\n"
                                  "lowp vec3 processed = vec3(\n"
                                  "		(rgb.r < 0.5 ? (2.0 * rgb.r * warmFilter.r) : (1.0 - 2.0 * (1.0 - rgb.r) * (1.0 - warmFilter.r))),//adjusting temperature\n"
                                  "		(rgb.g < 0.5 ? (2.0 * rgb.g * warmFilter.g) : (1.0 - 2.0 * (1.0 - rgb.g) * (1.0 - warmFilter.g))),\n"
                                  "		(rgb.b < 0.5 ? (2.0 * rgb.b * warmFilter.b) : (1.0 - 2.0 * (1.0 - rgb.b) * (1.0 - warmFilter.b))));\n"
                                  "return vec4(mix(rgb,processed,temp),source.a);\n"
                                  "}\n"
                                  "vec4 gamma(vec4 awb)\n"
                                  "{\n"
                                  "float gamma = 2.2;\n"
                                  "return vec4(pow(awb.rgb, vec3(1.0/gamma)),awb.a);\n"
                                  "}\n"
                                  "void main()\n"
                                  "{\n"
                                  "vec4 frag = texture(tex,v_Chord);\n"
                                  "vec3 color = frag.xyz;\n"
                                  "vec3 col_hsv = RGBtoHSV(color.rgb);\n"
                                  "col_hsv.y *= (u_saturation * 2.0); \n"
                                  "vec3 col_rgb = HSVtoRGB(col_hsv.rgb);\n"
                                  "vec4 final = vec4(col_rgb.rgb,1.0);\n"
                                  "vec4 contrast = contrast(final,u_contrast);\n"
                                  "vec4 brightness = brightness(contrast,u_brightness);\n"
                                  "float luminance = dot(brightness.rgb,vec3(0.3,0.3,0.3));\n"
                                  "float shadow = clamp((pow(luminance, 1.0/(u_shadow + 1.0)) + (-0.76)*pow(luminance, 2.0/(u_shadow + 1.0))) - luminance, 0.0, 1.0);\n"
                                  "float highlight = clamp((1.0 - (pow(1.0-luminance, 1.0/(2.0-u_highlight)) + (-0.8)*pow(1.0-luminance, 2.0/(2.0-u_highlight)))) - luminance, -1.0, 0.0);\n"
                                  "vec3 result = (luminance + shadow + highlight) * brightness.rgb / luminance;\n"
                                  "if(u_awb == 0.0)\n"
                                  "{\n"
                                  "vec4 awb = vec4(result.rgb,brightness.a);\n"
                                  "fragColor = awb;\n"
                                  "}\n"
                                  "else\n"
                                  "{\n"
                                  "vec4 awb = whiteBalance(vec4(result.rgb,brightness.a),0.00006 * (10000.0 - 5000.0),0.0);\n"
                                  "fragColor = awb;\n"
                                  "}\n"
                                  "}";

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
//    __android_log_print(ANDROID_LOG_DEBUG, "Camera", "%s", "Image available");
//        uint8_t *data = nullptr;
//        int len = 0;
//        AImage_getPlaneData(image, 0, &data, &len);

        int32_t planes;
        uint8_t *y = nullptr;
        uint8_t *u = nullptr;
        uint8_t *v = nullptr;
        int yl = 0;
        int ul = 0;
        int vl = 0;
        AImage_getNumberOfPlanes(image, &planes);
        AImage_getPlaneData(image, 0, &y, &yl);
        AImage_getPlaneData(image, 1, &u, &ul);
        AImage_getPlaneData(image, 2, &v, &vl);
        AImage_delete(image);

        uint8_t *yuvData = (uint8_t *) calloc((yl + ul + vl), sizeof(uint8_t));
        memcpy(yuvData, y, yl);
        memcpy(yuvData, u, ul);
        memcpy(yuvData, v, vl);

        JNIEnv *env;
        javaVM->AttachCurrentThread(&env, nullptr);
        jmethodID methodId = (*env).GetMethodID(activityClass, "runDetection", "([B)V");

        jbyteArray byteArray = env->NewByteArray(yl + ul + vl);
        (*env).SetByteArrayRegion(byteArray, 0, yl + ul + vl,
                                  reinterpret_cast<const jbyte *>(yuvData));
        (*env).CallVoidMethod(activityObj, methodId, byteArray);

//    env->ReleaseByteArrayElements(byteArray, reinterpret_cast<jbyte *>(data), JNI_ABORT);
        env->DeleteLocalRef(byteArray);
        javaVM->DetachCurrentThread();

//        const char *dirName = "/storage/emulated/0/pro/";
//        DIR *dir = opendir(dirName);
//        if (dir) {
//            closedir(dir);
//        } else {
//            std::string command = "mkdir -p ";
//            command += dirName;
//            system(command.c_str());
//        }
//
//        std::string fileName = dirName;
//        fileName += "preview.jpg";
//        FILE *imageFile = std::fopen(fileName.c_str(), "wb");
//        fwrite(data, 1, len, imageFile);
//        fclose(imageFile);
//        __android_log_print(ANDROID_LOG_DEBUG, "Camera", "%s", "Image saved");
    });
    processor.detach();
}

void createJpegReader() {
    media_status_t status = AImageReader_new(detectionWidth, detectionHeight,
                                             AIMAGE_FORMAT_YUV_420_888, 2, &imageReader);
    if (status != AMEDIA_OK) {
        __android_log_print(ANDROID_LOG_ERROR, "Camera Reader", "%d", status);
    }
    AImageReader_ImageListener listener{
            .context = nullptr,
            .onImageAvailable = imageCallback,
    };
    AImageReader_setImageListener(imageReader, &listener);
}

void createSurface() {
    AImageReader_getWindow(imageReader, &imageWindow);
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

//    jclass clazz = (*env).FindClass("com/demo/opengl/ui/CameraActivity");
//    jmethodID methodId = (*env).GetMethodID(clazz, "addExposureTime", "(JJ)V");
//    (*env).CallVoidMethod(object, methodId, minExposure, maxExposure);

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

    ACameraMetadata_getConstEntry(cameraMetadata, ACAMERA_CONTROL_AE_COMPENSATION_STEP, &entry);
    __android_log_print(ANDROID_LOG_DEBUG, "Camera Compensation", "%i", entry.data.i32[6]);
}

void createEglContext(JNIEnv *env, jobject surface) {
    static EGLint const attribute_list[] =
            {
                    EGL_RENDERABLE_TYPE, EGL_OPENGL_ES2_BIT,
                    EGL_RED_SIZE, 8,
                    EGL_GREEN_SIZE, 8,
                    EGL_BLUE_SIZE, 8,
                    EGL_DEPTH_SIZE, 24,
                    EGL_STENCIL_SIZE, 8,
                    EGL_NONE};

    static EGLint const context_list[] =
            {EGL_CONTEXT_CLIENT_VERSION, 3, EGL_NONE};

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
    previewWindow = ANativeWindow_fromSurface(env, surface);
    EGLCall(eglSurface = eglCreateWindowSurface(eglDisplay, eglConfig, previewWindow, nullptr))
    if (eglSurface == EGL_NO_SURFACE) {
        __android_log_print(ANDROID_LOG_ERROR, "Egl error", "%s", "Unable to create surface");
        int error = eglGetError();
        switch (error) {
            case EGL_BAD_CONFIG:
                __android_log_print(ANDROID_LOG_ERROR, "Egl error", "%s", "Bad config");
                break;
            case EGL_BAD_NATIVE_WINDOW:
                __android_log_print(ANDROID_LOG_ERROR, "Egl error", "%s", "Bad previewWindow");
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
    Camera(ACameraManager_openCamera(cameraManager, cameraID.c_str(), &stateCallbacks,
                                     &cameraDevice))
    cameraDetails(cameraID, env, object);
}

void captureImage() {
    Camera(ACameraCaptureSession_capture(session, &captureCallbacks, 1, &captureRequest, nullptr))
}

void autoMode() {
    uint8_t controlMode = ACAMERA_CONTROL_MODE_AUTO;
    Camera(ACaptureRequest_setEntry_u8(previewRequest, ACAMERA_CONTROL_MODE, 1, &controlMode))

    uint8_t controlAfMode = ACAMERA_CONTROL_AF_MODE_CONTINUOUS_PICTURE;
    Camera(ACaptureRequest_setEntry_u8(previewRequest, ACAMERA_CONTROL_AF_MODE, 1, &controlAfMode))

    uint8_t controlAWBMode = ACAMERA_CONTROL_AWB_MODE_AUTO;
    Camera(ACaptureRequest_setEntry_u8(previewRequest, ACAMERA_CONTROL_AWB_MODE, 1,
                                       &controlAWBMode))

    uint8_t controlAEMode = ACAMERA_CONTROL_AE_MODE_ON;
    Camera(ACaptureRequest_setEntry_u8(previewRequest, ACAMERA_CONTROL_AE_MODE, 1, &controlAEMode))

//    int32_t exposureCompensationValue = 0;
//    Camera(ACaptureRequest_setEntry_i32(previewRequest, ACAMERA_CONTROL_AE_EXPOSURE_COMPENSATION, 1, &exposureCompensationValue))

    // set repeating request
    Camera(ACameraCaptureSession_setRepeatingRequest(session, nullptr, 1, &previewRequest, nullptr))
}

void detectionMode() {
    uint8_t controlMode = ACAMERA_CONTROL_MODE_AUTO;
    Camera(ACaptureRequest_setEntry_u8(previewRequest, ACAMERA_CONTROL_MODE, 1, &controlMode))

    uint8_t controlAfMode = ACAMERA_CONTROL_AF_MODE_CONTINUOUS_PICTURE;
    Camera(ACaptureRequest_setEntry_u8(previewRequest, ACAMERA_CONTROL_AF_MODE, 1, &controlAfMode))

    uint8_t controlAWBMode = ACAMERA_CONTROL_AWB_MODE_AUTO;
    Camera(ACaptureRequest_setEntry_u8(previewRequest, ACAMERA_CONTROL_AWB_MODE, 1,
                                       &controlAWBMode))

    const uint8_t correctionMode = ACAMERA_COLOR_CORRECTION_MODE_TRANSFORM_MATRIX;
    Camera(ACaptureRequest_setEntry_u8(previewRequest, ACAMERA_COLOR_CORRECTION_MODE, 1,
                                       &correctionMode))

    const float gainValues[] = {1.0, 0.0, 0.0, 0.0};
    Camera(ACaptureRequest_setEntry_float(previewRequest, ACAMERA_COLOR_CORRECTION_GAINS, 1,
                                          reinterpret_cast<const float *>(&gainValues)))

    // Turn off AE to apply sensor sensitivity and exposure time
    uint8_t controlAEMode = ACAMERA_CONTROL_AE_MODE_OFF;
    Camera(ACaptureRequest_setEntry_u8(previewRequest, ACAMERA_CONTROL_AE_MODE, 1, &controlAEMode))

    int32_t exposureCompensationValue = -12;
    Camera(ACaptureRequest_setEntry_i32(previewRequest, ACAMERA_CONTROL_AE_EXPOSURE_COMPENSATION, 1,
                                        &exposureCompensationValue))

    int32_t orientation = 90;
    Camera(ACaptureRequest_setEntry_i32(previewRequest, ACAMERA_JPEG_ORIENTATION, 1, &orientation))

    const int32_t sensor = 800;
    Camera(ACaptureRequest_setEntry_i32(previewRequest, ACAMERA_SENSOR_SENSITIVITY, 1, &sensor))

    int64_t sensorExposureTime = 30000000;
    Camera(ACaptureRequest_setEntry_i64(previewRequest, ACAMERA_SENSOR_EXPOSURE_TIME, 1,
                                        &sensorExposureTime))

//    const int64_t frameDuration = 3000000;
//    camera_status_t sensorFrameDurationStatus = ACaptureRequest_setEntry_i64(previewRequest,ACAMERA_SENSOR_FRAME_DURATION,1,&frameDuration);

    float lensAperture = 1.5;
    Camera(ACaptureRequest_setEntry_float(previewRequest, ACAMERA_LENS_APERTURE, 1, &lensAperture))

    const uint8_t noiseMode = ACAMERA_NOISE_REDUCTION_MODE_MINIMAL;
    Camera(ACaptureRequest_setEntry_u8(previewRequest, ACAMERA_NOISE_REDUCTION_MODE, 1, &noiseMode))

    // set repeating request
    Camera(ACameraCaptureSession_setRepeatingRequest(session, nullptr, 1, &previewRequest, nullptr))
}

void startCamera(JNIEnv *env, jobject object) {
    // create container
    Camera(ACaptureSessionOutputContainer_create(&previewSessionOutputContainer))

    // Create ImageReader
    createJpegReader();
    createSurface();

    // Create preview capture request
    Camera(ACameraDevice_createCaptureRequest(cameraDevice, TEMPLATE_PREVIEW, &previewRequest))

    // Create capture request
    Camera(ACameraDevice_createCaptureRequest(cameraDevice, TEMPLATE_STILL_CAPTURE,
                                              &captureRequest))

    // Prepare preview output
    Camera(ACaptureSessionOutput_create(previewWindow, &previewSessionOutput))
    Camera(ACaptureSessionOutputContainer_add(previewSessionOutputContainer, previewSessionOutput))
    Camera(ACameraOutputTarget_create(previewWindow, &previewOutputTarget))
    Camera(ACaptureRequest_addTarget(previewRequest, previewOutputTarget))

    // Prepare image reader output
    Camera(ACaptureSessionOutput_create(imageWindow, &captureSessionOutput))
    Camera(ACaptureSessionOutputContainer_add(previewSessionOutputContainer, captureSessionOutput))
    Camera(ACameraOutputTarget_create(imageWindow, &captureOutputTarget))
    // Image reader output is set to preview request because we need ImageReader with preview surface
    // To Setup image reader with the capture request create a capture request and add target to capture request instead of Preview request
//    Camera(ACaptureRequest_addTarget(previewRequest, captureOutputTarget))

    // Create capture session
    Camera(ACameraDevice_createCaptureSession(cameraDevice, previewSessionOutputContainer,
                                              &sessionCallbacks, &session))

    detectionMode();
}

void destroy() {
    ACameraCaptureSession_stopRepeating(session);
    ANativeWindow_release(previewWindow);
    ANativeWindow_release(imageWindow);
    ACameraMetadata_free(cameraMetadata);
    ACaptureRequest_free(previewRequest);
//    ACameraDevice_close(cameraDevice);
    ACaptureRequest_free(captureRequest);
    ACameraCaptureSession_close(session);
    ACaptureSessionOutputContainer_remove(previewSessionOutputContainer, captureSessionOutput);
    ACaptureSessionOutputContainer_remove(previewSessionOutputContainer, previewSessionOutput);
    ACaptureSessionOutputContainer_free(previewSessionOutputContainer);
    ACaptureSessionOutputContainer_free(captureSessionOutputContainer);
    ACameraOutputTarget_free(captureOutputTarget);
    ACameraOutputTarget_free(previewOutputTarget);
    AImageReader_delete(imageReader);
    ACameraManager_delete(cameraManager);
}

void closeCamera() {
    ACameraDevice_close(cameraDevice);
}

static ALooper_callbackFunc looperCallback = {
};

extern "C" {

void Java_com_demo_opengl_ui_CameraActivity_setup(JNIEnv *jni, jobject object) {
    jni->GetJavaVM(&javaVM);
    jclass cls = jni->GetObjectClass(object);
    activityClass = (jclass) jni->NewGlobalRef(cls);
    activityObj = jni->NewGlobalRef(object);
    jmethodID id = jni->GetMethodID(cls, "getAssets", "()Landroid/content/res/AssetManager;");
    assetManager = AAssetManager_fromJava(jni, jni->CallObjectMethod(object, id));
}

void Java_com_demo_opengl_provider_CameraInterface_initialize(JNIEnv *jni, jobject object) {
    cameraManager = ACameraManager_create();
//    sensorManager = ASensorManager_getInstance();
//    const ASensor *accelerometerSensor = ASensorManager_getDefaultSensor(sensorManager, ASENSOR_TYPE_ACCELEROMETER);
//    const ASensorEventQueue *eventQueue = ASensorManager_createEventQueue(sensorManager, ALooper_forThread(), 0, looperCallback, nullptr);
    openCamera(jni, object);
}

void Java_com_demo_opengl_provider_CameraInterface_openCamera(JNIEnv *jni, jobject object) {
    openCamera(jni, object);
}

void Java_com_demo_opengl_provider_CameraInterface_closeCamera(JNIEnv *jni, jobject object) {
    closeCamera();
}

void Java_com_demo_opengl_provider_CameraInterface_changeExposure(JNIEnv *jni, jobject object,
                                                                  jint exposure) {
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

void Java_com_demo_opengl_provider_CameraInterface_destroy(JNIEnv *jni, jobject object) {
    destroy();
}

void Java_com_demo_opengl_provider_CameraInterface_capture(JNIEnv *jni, jobject object) {
    captureImage();
}

void createVideoProgram() {

    const std::string vert = "#version 320 es\n"
                             "in vec3 position;\n"
                             "in vec2 texChords;\n"
                             "uniform mat4 u_MVP;"
                             "uniform mat4 texMatrix;\n"
                             "out vec2 v_Chord;\n"
                             "void main()\n"
                             "{\n"
                             "    v_Chord = (texMatrix * vec4(texChords.x, texChords.y, 0.0, 1.0)).xy;\n"
                             "    gl_Position = u_MVP * vec4(position, 1.0); \n"
                             "}";

    const std::string frag = "#version 320 es\n"
                             "#extension GL_OES_EGL_image_external_essl3:require\n"
                             "precision highp float;\n"
                             "in lowp vec2 v_Chord;\n"
                             "uniform samplerExternalOES tex;"
                             "out vec4 fragColor;\n"
                             "void main()\n"
                             "{\n"
                             "    vec4 c = texture(tex, v_Chord);\n"
                             "    float rbAverage = c.r * 0.5 + c.b * 0.5;\n"
                             "    float gDelta = c.g - rbAverage;\n"
                             "    c.a = 1.0 - smoothstep(0.0, 0.25, gDelta);\n"
                             "    c.a = c.a * c.a * c.a;\n"
                             "    fragColor = c;\n"
                             "}";

    // Video Program
    GLCall(videoProgram = glCreateProgram())
    GLCall(videoShader = new Shader(videoProgram, vert, frag))
    GLCall(glLinkProgram(videoProgram))

    GLboolean isVideoProgram = glIsProgram(videoProgram);
    if (isVideoProgram == GL_FALSE) {
        __android_log_print(ANDROID_LOG_ERROR, "OpenGL isProgram", "%s", "Failed");
    }
    GLint videoStatus;
    glGetProgramiv(meshProgram, GL_LINK_STATUS, &videoStatus);
    if (videoStatus == GL_FALSE) {
        __android_log_print(ANDROID_LOG_ERROR, "OpenGL program status", "%s", "Failed");
        GLsizei logLength;
        GLchar log[1024];
        glGetProgramInfoLog(program, sizeof(log), &logLength, log);
        __android_log_print(ANDROID_LOG_ERROR, "OpenGL program status", "%s", log);
    }
    float vertices[] = {
            -1.0f, -1.0f, 0.0f, 0.0f, 0.0f,
            -1.0, 1.0f, 0.0f, 0.0f, 1.0f,
            1.0f, 1.0f, 0.0f, 1.0f, 1.0f,
            1.0f, -1.0f, 0.0f, 1.0f, 0.0f
    };

    unsigned int indices[] = {
            2, 1, 0, 0, 3, 2
    };
    videoVertexBuffer = new VertexBuffer(vertices, sizeof(vertices));
    videoIndexBuffer = new IndexBuffer(indices, sizeof(indices));
}

void drawVideoProgram(float *mvp, float *texMatrix) {
    GLCall(glUseProgram(videoProgram))

    GLCall(glUniformMatrix4fv(videoShader->getUniformLocation("u_MVP"), 1, false, mvp))
    GLCall(glUniformMatrix4fv(videoShader->getUniformLocation("texMatrix"), 1, false, texMatrix))

    GLCall(glActiveTexture(GL_TEXTURE1))
    GLCall(glBindTexture(GL_TEXTURE_EXTERNAL_OES, videoTextureId))
    GLCall(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR))
    GLCall(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR))
    GLCall(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE))
    GLCall(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE))

    GLCall(videoShader->setUniform1i(videoShader->getUniformLocation("tex"), 1))

    GLCall(videoVertexBuffer->bind())
    GLCall(videoIndexBuffer->bind())

    GLCall(GLuint position = videoShader->getAttributeLocation("position"))
    GLCall(GLuint chords = videoShader->getAttributeLocation("texChords"))
    GLCall(glEnableVertexAttribArray(position))
    GLCall(glVertexAttribPointer(position, 3, GL_FLOAT, false, 5 * sizeof(float),
                                 (GLvoid *) nullptr))
    GLCall(glEnableVertexAttribArray(chords))
    GLCall(glVertexAttribPointer(chords, 2, GL_FLOAT, false, 5 * sizeof(float),
                                 (GLvoid *) (3 * sizeof(float))))

    GLCall(glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, nullptr))

    GLCall(glDisableVertexAttribArray(position))
    GLCall(glDisableVertexAttribArray(chords))
}

void Java_com_demo_opengl_provider_CameraInterface_onSurfaceCreated(JNIEnv *jni,
                                                                    jobject object,
                                                                    jintArray textureId,
                                                                    jobject surface,
                                                                    jint width,
                                                                    jint height) {

    GLCall(glClearColor(0, 0, 0, 0))
    GLCall(glEnable(GL_CULL_FACE))
//    GLCall(glCullFace(GL_BACK))
//    GLCall(glFrontFace(GL_CW))
    // GLCall(glEnable(GL_DEPTH_TEST))
    // Enable blend when required and disable it again
    GLCall(glEnable(GL_BLEND))
    // GLCall(glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA))

    int *ids = jni->GetIntArrayElements(textureId, nullptr);

    textureID = ids[0];
    videoTextureId = ids[1];
    cameraWidth = width;
    cameraHeight = height;

    // Create previewWindow with surface
    previewWindow = ANativeWindow_fromSurface(jni, surface);
    // createEglContext(jni, surface);

    float vertices[] = {
            -1.0f, -1.0f, 0.0f, 0.0f, 0.0f,
            -1.0, 1.0f, 0.0f, 0.0f, 1.0f,
            1.0f, 1.0f, 0.0f, 1.0f, 1.0f,
            1.0f, -1.0f, 0.0f, 1.0f, 0.0f
    };

    unsigned int indices[] = {
            2, 1, 0, 0, 3, 2
    };

    __android_log_print(ANDROID_LOG_ERROR, "OpenGL version", "%s", glGetString(GL_VERSION));
    __android_log_print(ANDROID_LOG_ERROR, "OpenGL shader version", "%s",
                        glGetString(GL_SHADING_LANGUAGE_VERSION));
    GLCall(program = glCreateProgram())
    GLCall(Shader shader = Shader(program, vertexShaderCode, fragmentShaderCode))

//    GLCall(glBindAttribLocation(program, 0, "position"))

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

    GLCall(glActiveTexture(GL_TEXTURE2))
    GLCall(glGenTextures(1, &textureLutReferenceID))
    GLCall(glBindTexture(GL_TEXTURE_2D, textureLutReferenceID))
    GLCall(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR))
    GLCall(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR))
    GLCall(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE))
    GLCall(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE))

    // Lut texture
    int tw;
    int th;
    int channels;
    unsigned char *buffer;
    std::string path = "/storage/emulated/0/default.png";
    stbi_set_flip_vertically_on_load(true);
    buffer = stbi_load(path.c_str(), &tw, &th, &channels, 4);
    GLCall(glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, tw, th, 0, GL_RGBA, GL_UNSIGNED_BYTE, buffer))
    if (buffer) {
        stbi_image_free(buffer);
    } else {
        __android_log_print(ANDROID_LOG_ERROR, "Lut texture", "%s", "Failed");
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
    GLCall(highLightLocation = glGetUniformLocation(program, "u_highlight"))
    GLCall(shadowLocation = glGetUniformLocation(program, "u_shadow"))
    GLCall(awbLocation = glGetUniformLocation(program, "u_awb"))
    GLCall(lutTextureLocation = glGetUniformLocation(program, "textureLut"))

    startCamera(jni, object);

    // create Program using mesh(Assimp)
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
    GLCall(meshShader = new Shader(meshProgram, vertexSource, fragSource))
    GLCall(glLinkProgram(meshProgram))

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
        glGetProgramInfoLog(program, sizeof(log), &logLength, log);
        __android_log_print(ANDROID_LOG_ERROR, "OpenGL program status", "%s", log);
    }

    mesh = new Mesh(assetManager);
    bool isLoaded = mesh->LoadMesh("boblampclean.md5mesh");
    __android_log_print(ANDROID_LOG_ERROR, "Model loading", "loaded %d", isLoaded);

    meshCamera = new CameraView();
    GLCall(meshCamera->setLocation(meshProgram, "camera"))

    // createVideoProgram();
}

void Java_com_demo_opengl_provider_CameraInterface_onSurfaceChanged(JNIEnv *jni,
                                                                    jobject object,
                                                                    jint width,
                                                                    jint height) {
    GLCall(glViewport(0, 0, width, height))

    windowWidth = width;
    windowHeight = height;
//    ANativeWindow_acquire(previewWindow);
//    ANativeWindow_setBuffersGeometry(previewWindow, windowWidth, windowHeight,
//                                     WINDOW_FORMAT_RGBA_8888);

}
void
Java_com_demo_opengl_provider_CameraInterface_onDrawFrame(JNIEnv *jni, jobject object,
                                                          jfloatArray array,
                                                          jfloatArray videoArray,
                                                          jfloat saturation,
                                                          jfloat contrast,
                                                          jfloat brightness,
                                                          jfloat highlight,
                                                          jfloat shadow,
                                                          jfloatArray orientation,
                                                          jfloatArray distance) {
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    GLCall(glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT))

    GLCall(glUseProgram(program))

    float aspect = windowWidth > windowHeight ? float(windowWidth) / float(windowHeight) :
                   float(windowHeight) / float(windowWidth);

    float mvp[] = {
            1.0f, 0.0f, 0.0f, 0.0f,
            0.0f, 1.0f, 0.0f, 0.0f,
            0.0f, 0.0f, 1.0f, 0.0f,
            0.0f, 0.0f, 0.0f, 1.0f,
    };
    if (windowWidth < windowHeight) // portrait
        ortho(mvp, -1.0f, 1.0f, -1.0, 1.0, -1.0f, 1.0f);
    else // landscape
        ortho(mvp, -aspect, aspect, -1.0f, 1.0f, -1.0f, 1.0f);


    GLCall(glUniformMatrix4fv(mvpLocation, 1, false, mvp))

    GLCall(glActiveTexture(GL_TEXTURE0))
    GLCall(glBindTexture(GL_TEXTURE_EXTERNAL_OES, textureID))
    GLCall(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR))
    GLCall(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR))
    GLCall(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE))
    GLCall(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE))

    float *tm = jni->GetFloatArrayElements(array, nullptr);
    GLCall(glUniformMatrix4fv(matrixLocation, 1, false, tm))
    jni->ReleaseFloatArrayElements(array, tm, 0);

    GLCall(glUniform1i(textureLocation, 0))

    GLCall(glActiveTexture(GL_TEXTURE2))
    GLCall(glBindTexture(GL_TEXTURE_2D, textureLutReferenceID))
    GLCall(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR))
    GLCall(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR))
    GLCall(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE))
    GLCall(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE))
    GLCall(glUniform1i(lutTextureLocation, 2))

    GLCall(glUniform1f(saturationLocation, saturation))
    GLCall(glUniform1f(contrastLocation, contrast))
    GLCall(glUniform1f(brightnessLocation, brightness))
    GLCall(glUniform1f(highLightLocation, highlight))
    GLCall(glUniform1f(shadowLocation, shadow))
    GLCall(glUniform1f(awbLocation, applyAwb))

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

    // Mesh program
    GLCall(glUseProgram(meshProgram))
    glm::mat4 projection = glm::perspective(45.0f,
                                            (float) windowWidth / (float) windowHeight,
                                            0.1f,
                                            1000.0f);
    glm::mat4 translate = glm::translate(glm::mat4(1.0), glm::vec3(0.0f, 0.0f, 30.0f));
    glm::mat4 rotationX = glm::rotate(glm::mat4(1.0f), glm::radians(270.0f),
                                      glm::vec3(1.0f, 0.0f, 0.0f));
    glm::mat4 rotationY = glm::rotate(glm::mat4(1.0f), glm::radians(180.0f),
                                      glm::vec3(0.0f, 0.0f, 1.0f));
    glm::mat4 scale = glm::scale(glm::mat4(1.0), glm::vec3(0.2, 0.2, 0.2));
    glm::mat4 model = translate * rotationX * rotationY * scale;
    GLCall(GLuint projectionLocation = meshShader->getUniformLocation("projection"))
    GLCall(GLuint modelLocation = meshShader->getUniformLocation("model"))
    meshCamera->useCamera();

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

    Vector3f localPos = mesh->GetWorldTransform().WorldPosToLocalPos(meshCamera->getCameraPos());
    GLCall(GLuint localCameraPosLocation = meshShader->getUniformLocation("gCameraLocalPos"))
    GLCall(glUniform3f(localCameraPosLocation, localPos.x, localPos.y, localPos.z))

//    for (unsigned int i = 0; i < mesh->m_Entries.size(); i++) {
//        GLCall(glBindBuffer(GL_ARRAY_BUFFER, mesh->m_Entries[i].VB))
//        GLCall(glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mesh->m_Entries[i].IB))
//
//        GLCall(glEnableVertexAttribArray(0))
//        GLCall(glEnableVertexAttribArray(1))
//        GLCall(glEnableVertexAttribArray(2))
//
//        GLCall(glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (const GLvoid *) 0))
//        GLCall(glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (const GLvoid *) 12))
//
//        const unsigned int MaterialIndex = mesh->m_Entries[i].MaterialIndex;
//        if (MaterialIndex < mesh->m_Textures.size() && mesh->m_Textures[MaterialIndex]) {
//            mesh->m_Textures[MaterialIndex]->bind();
//            GLCall(meshShader->setUniform1i(meshTextureLocation,
//                                            mesh->m_Textures[MaterialIndex]->getSlot()))
//        }
//        GLCall(glDrawElements(GL_TRIANGLES, mesh->m_Entries[i].NumIndices, GL_UNSIGNED_INT,
//                              nullptr))
//
//        GLCall(glDisableVertexAttribArray(0))
//        GLCall(glDisableVertexAttribArray(1))
//        GLCall(glDisableVertexAttribArray(2))
//    }
    mesh->Render(meshProgram);

    float *videoMvp = jni->GetFloatArrayElements(videoArray, nullptr);
    // drawVideoProgram(mvp, videoMvp);
    jni->ReleaseFloatArrayElements(videoArray, videoMvp, 0);
}

void
Java_com_demo_opengl_provider_CameraInterface_changeMode(JNIEnv *jni, jobject object, jint mode) {
    if (mode == 0) {
        autoMode();
        applyAwb = 0;
    } else if (mode == 1) {
        detectionMode();
        applyAwb = 1;
    }
}
}
