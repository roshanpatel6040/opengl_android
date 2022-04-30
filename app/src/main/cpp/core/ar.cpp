//
// Created by Roshan on 23/04/22.
//

#include <jni.h>
#include <ext.hpp>
#include <android/log.h>
#include "ar.h"
#include "thread"

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
    // GLCall(glEnable(GL_CULL_FACE))
    backgroundRenderer.InitializeGlContent(assetManager, 0 /*TODO create texture id for depth*/);
    circleRenderer.CreateOnGlThread(assetManager);
    // bobLampCleanRenderer.createOnGlThread(assetManager);
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

    if (session == nullptr) {
        LOGE("Session null");
        return;
    };

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

    // Render Andy objects.
    glm::mat4 model_mat(1.0f);
    for (auto &colored_anchor : anchors_) {
        ArTrackingState tracking_state = AR_TRACKING_STATE_STOPPED;
        ArAnchor_getTrackingState(session, colored_anchor.anchor,
                                  &tracking_state);
        if (tracking_state == AR_TRACKING_STATE_TRACKING) {
            // UpdateAnchorColor(&colored_anchor);
            // Render object only if the tracking state is AR_TRACKING_STATE_TRACKING.
            // util::GetTransformMatrixFromAnchor(*colored_anchor.anchor, ar_session_,&model_mat);
            // andy_renderer_.Draw(projection_mat, view_mat, model_mat, color_correction, colored_anchor.color);
            circleRenderer.drawCircle(projection_mat, view_mat);
            // bobLampCleanRenderer.Draw(projection_mat, view_mat);
        }
    }
}

void ArApplication::onTouched(float x, float y) {
    if (arFrame != nullptr && session != nullptr) {
        ArHitResultList *hit_result_list = nullptr;
        ArHitResultList_create(session, &hit_result_list);
        // CHECK(hit_result_list);
        if (true) {
            ArFrame_hitTestInstantPlacement(session, arFrame, x, y,
                                            2.0,
                                            hit_result_list);
        } else {
            ArFrame_hitTest(session, arFrame, x, y, hit_result_list);
        }

        int32_t hit_result_list_size = 0;
        ArHitResultList_getSize(session, hit_result_list,
                                &hit_result_list_size);

        // The hitTest method sorts the resulting list by distance from the camera,
        // increasing.  The first hit result will usually be the most relevant when
        // responding to user input.

        ArHitResult *ar_hit_result = nullptr;
        for (int32_t i = 0; i < hit_result_list_size; ++i) {
            ArHitResult *ar_hit = nullptr;
            ArHitResult_create(session, &ar_hit);
            ArHitResultList_getItem(session, hit_result_list, i, ar_hit);

            if (ar_hit == nullptr) {
                // LOGE("HelloArApplication::OnTouched ArHitResultList_getItem error");
                return;
            }

            ArTrackable *ar_trackable = nullptr;
            ArHitResult_acquireTrackable(session, ar_hit, &ar_trackable);
            ArTrackableType ar_trackable_type = AR_TRACKABLE_NOT_VALID;
            ArTrackable_getType(session, ar_trackable, &ar_trackable_type);
            // Creates an anchor if a plane or an oriented point was hit.
            if (AR_TRACKABLE_PLANE == ar_trackable_type) {
                ArPose *hit_pose = nullptr;
                ArPose_create(session, nullptr, &hit_pose);
                ArHitResult_getHitPose(session, ar_hit, hit_pose);
                int32_t in_polygon = 0;
                ArPlane *ar_plane = ArAsPlane(ar_trackable);
                ArPlane_isPoseInPolygon(session, ar_plane, hit_pose, &in_polygon);

                // Use hit pose and camera pose to check if hittest is from the
                // back of the plane, if it is, no need to create the anchor.
                ArPose *camera_pose = nullptr;
                ArPose_create(session, nullptr, &camera_pose);
                ArCamera *ar_camera;
                ArFrame_acquireCamera(session, arFrame, &ar_camera);
                ArCamera_getPose(session, ar_camera, camera_pose);
                ArCamera_release(ar_camera);
                // float normal_distance_to_plane = CalculateDistanceToPlane(*session, *hit_pose, *camera_pose);

                ArPose_destroy(hit_pose);
                ArPose_destroy(camera_pose);

                if (!in_polygon /*|| normal_distance_to_plane < 0*/) {
                    continue;
                }

                ar_hit_result = ar_hit;
                break;
            } else if (AR_TRACKABLE_POINT == ar_trackable_type) {
                ArPoint *ar_point = ArAsPoint(ar_trackable);
                ArPointOrientationMode mode;
                ArPoint_getOrientationMode(session, ar_point, &mode);
                if (AR_POINT_ORIENTATION_ESTIMATED_SURFACE_NORMAL == mode) {
                    ar_hit_result = ar_hit;
                    break;
                }
            } else if (AR_TRACKABLE_INSTANT_PLACEMENT_POINT == ar_trackable_type) {
                ar_hit_result = ar_hit;
            } else if (AR_TRACKABLE_DEPTH_POINT == ar_trackable_type) {
                // ArDepthPoints are only returned if ArConfig_setDepthMode() is called
                // with AR_DEPTH_MODE_AUTOMATIC.
                ar_hit_result = ar_hit;
            }
        }

        if (ar_hit_result) {
            // Note that the application is responsible for releasing the anchor
            // pointer after using it. Call ArAnchor_release(anchor) to release.
            ArAnchor *anchor = nullptr;
            if (ArHitResult_acquireNewAnchor(session, ar_hit_result, &anchor) !=
                AR_SUCCESS) {
                // LOGE("HelloArApplication::OnTouched ArHitResult_acquireNewAnchor error");
                return;
            }

            ArTrackingState tracking_state = AR_TRACKING_STATE_STOPPED;
            ArAnchor_getTrackingState(session, anchor, &tracking_state);
            if (tracking_state != AR_TRACKING_STATE_TRACKING) {
                ArAnchor_release(anchor);
                return;
            }

            if (anchors_.size() >= 20) {
                ArAnchor_release(anchors_[0].anchor);
                ArTrackable_release(anchors_[0].trackable);
                anchors_.erase(anchors_.begin());
            }

            ArTrackable *ar_trackable = nullptr;
            ArHitResult_acquireTrackable(session, ar_hit_result, &ar_trackable);
            // Assign a color to the object for rendering based on the trackable type
            // this anchor attached to. For AR_TRACKABLE_POINT, it's blue color, and
            // for AR_TRACKABLE_PLANE, it's green color.
            ColoredAnchor colored_anchor;
            colored_anchor.anchor = anchor;
            colored_anchor.trackable = ar_trackable;

            // UpdateAnchorColor(&colored_anchor);
            anchors_.push_back(colored_anchor);

            ArHitResult_destroy(ar_hit_result);
            ar_hit_result = nullptr;

            ArHitResultList_destroy(hit_result_list);
            hit_result_list = nullptr;
        }
    }
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