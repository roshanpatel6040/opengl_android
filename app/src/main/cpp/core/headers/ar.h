//
// Created by Roshan on 23/04/22.
//

#ifndef OPENGL_AR_H
#define OPENGL_AR_H

#include "GLES3/gl32.h"
#include <android/asset_manager.h>
#include "../../../../../arcore/include/arcore_c_api.h"
#include "background_renderer.h"
#include <glm.hpp>
#include <ext.hpp>
#include <Renderer.h>

class ArApplication {
public:
    ArApplication(AAssetManager *manager);

    void onCreate();

    void onResume(JNIEnv *env, void *context, void *activity);

    void onPause();

    void onDestroy();

    void onSurfaceCreated();

    void onSurfaceChanged(int32_t rotation, int32_t w, int32_t h);

    void onDraw();

    void onTouched(float x, float y);

private:
    AAssetManager *assetManager;
    ArSession *session = nullptr;
    ArFrame *arFrame = nullptr;
    int32_t width = 1;
    int32_t height = 1;
    int32_t displayRotation = 1;
    bool calculate_uv_transform_ = false;

    BackgroundRenderer backgroundRenderer;

    void configureSession();

    glm::mat3 GetTextureTransformMatrix(const ArSession *session, const ArFrame *frame);
};

#endif //OPENGL_AR_H
