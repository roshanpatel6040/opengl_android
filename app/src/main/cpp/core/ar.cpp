//
// Created by Roshan on 23/04/22.
//

#include <jni.h>
#include <ext.hpp>
#include "ar.h"

ArApplication::ArApplication(AAssetManager *manager) : assetManager(manager) {

}

void ArApplication::onCreate() {

}

void ArApplication::onResume(JNIEnv *env, void *context, void *activity) {
    if (session == nullptr) {
        ArSession_create(env, context, &session);
        configureSession();
        ArFrame_create(session, &arFrame);
        ArSession_setDisplayGeometry(session, displayRotation, width, height);
    }
    ArStatus status = ArSession_resume(session);
    if (status != AR_SUCCESS) {
        // TODO add log
    }
}

void ArApplication::configureSession() {
    ArConfig *ar_config = nullptr;
    ArConfig_create(session, &ar_config);
    ArConfig_setDepthMode(session, ar_config, AR_DEPTH_MODE_AUTOMATIC);
    ArConfig_setInstantPlacementMode(session, ar_config, AR_INSTANT_PLACEMENT_MODE_LOCAL_Y_UP);
    ArConfig_setFocusMode(session, ar_config, AR_FOCUS_MODE_AUTO);
    ArSession_configure(session, ar_config);
    ArConfig_destroy(ar_config);
}

void ArApplication::onPause() {
    ArSession_pause(session);
}

void ArApplication::onDestroy() {
    ArSession_destroy(session);
    ArFrame_destroy(arFrame);
}

void ArApplication::onSurfaceCreated() {
    backgroundRenderer.InitializeGlContent(assetManager, 0 /*TODO create texture id for depth*/);
}

void ArApplication::onSurfaceChanged(int32_t rotation, int32_t w, int32_t h) {
    GLCall(glViewport(0, 0, w, h))
    width = w;
    height = h;
    displayRotation = rotation;
    if (session != nullptr) {
        ArSession_setDisplayGeometry(session, displayRotation, width, height);
    }
}

void ArApplication::onDraw() {
    GLCall(glClearColor(1, 0, 0, 0))
    GLCall(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT)

    if (session == nullptr) return;

    // Camera texture id
    ArSession_setCameraTextureName(session, backgroundRenderer.GetTextureId());

    if (ArSession_update(session, arFrame) != AR_SUCCESS) {
        // Failed to update frame
    }

    ArCamera *ar_camera;
    ArFrame_acquireCamera(session, arFrame, &ar_camera);

    int32_t geometry_changed = 0;
    ArFrame_getDisplayGeometryChanged(session, arFrame, &geometry_changed);
    if (geometry_changed != 0 || !calculate_uv_transform_) {
        // The UV Transform represents the transformation between screenspace in
        // normalized units and screenspace in units of pixels.  Having the size of
        // each pixel is necessary in the virtual object shader, to perform
        // kernel-based blur effects.
        calculate_uv_transform_ = false;
        glm::mat3 transform = GetTextureTransformMatrix(session, arFrame);
        // andy_renderer_.SetUvTransformMatrix(transform);
    }

    glm::mat4 view_mat;
    glm::mat4 projection_mat;
    ArCamera_getViewMatrix(session, ar_camera, glm::value_ptr(view_mat));
    ArCamera_getProjectionMatrix(session, ar_camera, 0.1f, 100.f, glm::value_ptr(projection_mat));

    backgroundRenderer.Draw(session, arFrame, false);

    ArTrackingState camera_tracking_state;
    ArCamera_getTrackingState(session, ar_camera, &camera_tracking_state);
    ArCamera_release(ar_camera);

    // If the camera isn't tracking don't bother rendering other objects.
    if (camera_tracking_state != AR_TRACKING_STATE_TRACKING) {
        return;
    }
}

void ArApplication::onTouched(float x, float y) {

}

glm::mat3 ArApplication::GetTextureTransformMatrix(const ArSession *session, const ArFrame *frame) {
    float frameTransform[6];
    float uvTransform[9];
    // XY pairs of coordinates in NDC space that constitute the origin and points
    // along the two principal axes.
    const float ndcBasis[6] = {0, 0, 1, 0, 0, 1};
    ArFrame_transformCoordinates2d(
            session, frame, AR_COORDINATES_2D_OPENGL_NORMALIZED_DEVICE_COORDINATES, 3,
            ndcBasis, AR_COORDINATES_2D_TEXTURE_NORMALIZED, frameTransform);

    // Convert the transformed points into an affine transform and transpose it.
    float ndcOriginX = frameTransform[0];
    float ndcOriginY = frameTransform[1];
    uvTransform[0] = frameTransform[2] - ndcOriginX;
    uvTransform[1] = frameTransform[3] - ndcOriginY;
    uvTransform[2] = 0;
    uvTransform[3] = frameTransform[4] - ndcOriginX;
    uvTransform[4] = frameTransform[5] - ndcOriginY;
    uvTransform[5] = 0;
    uvTransform[6] = ndcOriginX;
    uvTransform[7] = ndcOriginY;
    uvTransform[8] = 1;

    return glm::make_mat3(uvTransform);
}