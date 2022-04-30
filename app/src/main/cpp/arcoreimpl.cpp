//
// Created by Roshan on 21/04/22.
//
#include <jni.h>
#include <ar.h>
#include <android/asset_manager_jni.h>
#include <arcore_c_api.h>
#include <vector>
#include "stb_image/stb_image.cpp"

ArApplication *application;

extern "C"
JNIEXPORT void JNICALL
Java_com_demo_opengl_provider_ArcoreInterface_onSurfaceCreated(JNIEnv *env, jobject thiz) {
    application->onSurfaceCreated();
}
extern "C"
JNIEXPORT void JNICALL
Java_com_demo_opengl_provider_ArcoreInterface_onSurfaceChanged(JNIEnv *env, jobject thiz,
                                                               jint rotation, jint width,
                                                               jint height) {
    application->onSurfaceChanged(rotation, width, height);
}
extern "C"
JNIEXPORT void JNICALL
Java_com_demo_opengl_provider_ArcoreInterface_onDrawFrame(JNIEnv *env, jobject thiz) {
    application->onDraw();
}
extern "C"
JNIEXPORT void JNICALL
Java_com_demo_opengl_provider_ArcoreInterface_onCreate(JNIEnv *env, jobject thiz,
                                                       jobject assetManager) {
    AAssetManager *manager = AAssetManager_fromJava(env, assetManager);
    application = new ArApplication(manager);
}
extern "C"
JNIEXPORT void JNICALL
Java_com_demo_opengl_provider_ArcoreInterface_onResume(JNIEnv *env, jobject thiz, jobject context,
                                                       jobject activity) {
    application->onResume(env, context, activity);
}
extern "C"
JNIEXPORT void JNICALL
Java_com_demo_opengl_provider_ArcoreInterface_onPause(JNIEnv *env, jobject thiz) {
    application->onPause();
}
extern "C"
JNIEXPORT void JNICALL
Java_com_demo_opengl_provider_ArcoreInterface_onDestroy(JNIEnv *env, jobject thiz) {
    delete application;
}
extern "C"
JNIEXPORT void JNICALL
Java_com_demo_opengl_provider_ArcoreInterface_onTouched(JNIEnv *env, jobject thiz, jfloat x,
                                                        jfloat y) {
    application->onTouched(x,y);
}