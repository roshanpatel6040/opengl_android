//
// Created by Roshan on 23/04/22.
//

#include <jni.h>
#include <ext.hpp>
#include <android/log.h>
#include "ar.h"
#include "thread"
#include <../glm/gtx/matrix_decompose.hpp>

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
    depthTexture.CreateOnGlThread();
    backgroundRenderer.InitializeGlContent(assetManager, depthTexture.GetTextureId());
    planeRenderer.InitializeGlContent(assetManager);
    circleRenderer.CreateOnGlThread(assetManager);
    pickTexture.Init(1080, 2340);
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

    int32_t isDepthModeSupported;
    ArSession_isDepthModeSupported(session, AR_DEPTH_MODE_AUTOMATIC, &isDepthModeSupported);
    if (isDepthModeSupported) {
        depthTexture.UpdateWithDepthImageOnGlThread(*session, *arFrame);
    }

    // Update and render planes.
    ArTrackableList *plane_list = nullptr;
    ArTrackableList_create(session, &plane_list);

    ArTrackableType plane_tracked_type = AR_TRACKABLE_PLANE;
    ArSession_getAllTrackables(session, plane_tracked_type, plane_list);

    int32_t plane_list_size = 0;
    ArTrackableList_getSize(session, plane_list, &plane_list_size);
    plane_count_ = plane_list_size;

    for (int i = 0; i < plane_list_size; ++i) {
        ArTrackable *ar_trackable = nullptr;
        ArTrackableList_acquireItem(session, plane_list, i, &ar_trackable);
        ArPlane *ar_plane = ArAsPlane(ar_trackable);
        ArTrackingState out_tracking_state;
        ArTrackable_getTrackingState(session, ar_trackable,
                                     &out_tracking_state);

        ArPlane *subsume_plane;
        ArPlane_acquireSubsumedBy(session, ar_plane, &subsume_plane);
        if (subsume_plane != nullptr) {
            ArTrackable_release(ArAsTrackable(subsume_plane));
            ArTrackable_release(ar_trackable);
            continue;
        }

        if (ArTrackingState::AR_TRACKING_STATE_TRACKING != out_tracking_state) {
            ArTrackable_release(ar_trackable);
            continue;
        }

        planeRenderer.Draw(projection_mat, view_mat, *session, *ar_plane);
        ArTrackable_release(ar_trackable);
    }

    ArTrackableList_destroy(plane_list);
    plane_list = nullptr;

    glDisable(GL_BLEND);
    // glDisable(GL_DEPTH_TEST);
    pickTexture.EnableWriting();
    glClearColor(0.1, 0.1, 0.1, 1.0);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    // glm::mat4 model(1.0f);
    for (int i = 0; i < anchors_.size(); i++) {
        auto &colored_anchor = anchors_[i];
        ArTrackingState tracking_state = AR_TRACKING_STATE_STOPPED;
        ArAnchor_getTrackingState(session, colored_anchor.anchor,
                                  &tracking_state);
        if (tracking_state == AR_TRACKING_STATE_TRACKING) {
            // UpdateAnchorColor(&colored_anchor);
            // Render object only if the tracking state is AR_TRACKING_STATE_TRACKING.
            // GetTransformMatrixFromAnchor(*colored_anchor.anchor, session, &model);
            // andy_renderer_.Draw(projection_mat, view_mat, model_mat, color_correction, colored_anchor.color);
            circleRenderer.drawCircle(i + 1, false, projection_mat, view_mat, colored_anchor.modelTransformation);
            // bobLampCleanRenderer.Draw(projection_mat, view_mat, model);
        }
    }
    pickTexture.DisableWriting();
    glEnable(GL_BLEND);
    // glEnable(GL_DEPTH_TEST);
    for (int i = 0; i < anchors_.size(); i++) {
        auto &colored_anchor = anchors_[i];
        ArTrackingState tracking_state = AR_TRACKING_STATE_STOPPED;
        ArAnchor_getTrackingState(session, colored_anchor.anchor,
                                  &tracking_state);
        if (tracking_state == AR_TRACKING_STATE_TRACKING) {
            // UpdateAnchorColor(&colored_anchor);
            // Render object only if the tracking state is AR_TRACKING_STATE_TRACKING.
            // GetTransformMatrixFromAnchor(*colored_anchor.anchor, session, &model);
            // andy_renderer_.Draw(projection_mat, view_mat, model_mat, color_correction, colored_anchor.color);
            circleRenderer.drawCircle(i + 1, colored_anchor.currentlySelected, projection_mat, view_mat, colored_anchor.modelTransformation);
            // bobLampCleanRenderer.Draw(projection_mat, view_mat, model);
        }
    }
}

void ArApplication::onTouched(float x, float y) {
    int id = pickTexture.ReadPixel(1, (int) x, height - (int) y);
    LOGE("Anchor Identity %d", id);
    bool newObjectClicked = false;
    for (int i = 0; i < anchors_.size(); ++i) {
        ColoredAnchor &anchor = anchors_[i];
        if (i + 1 == id) {
            anchor.currentlySelected = true;
            newObjectClicked = true;
        } else {
            anchor.currentlySelected = false;
        }
    }
    if (newObjectClicked) {
        return;
    }

    /*glm::vec3 ray_origin;
    glm::vec3 ray_direction;
    ArCamera *ar_camera;
    ArFrame_acquireCamera(session, arFrame, &ar_camera);
    glm::mat4 view_mat;
    glm::mat4 projection_mat;
    ArCamera_getViewMatrix(session, ar_camera, glm::value_ptr(view_mat));
    ArCamera_release(ar_camera);
    ArCamera_getProjectionMatrix(session, ar_camera, 0.1f, 100.f, glm::value_ptr(projection_mat));
    ScreenPosToWorldRay((int) x, (int) y, width, height, view_mat, projection_mat, ray_origin, ray_direction);

    for (int i = 0; i < anchors_.size(); i++) {
        ColoredAnchor anchor = anchors_[i];
        float intersection_distance; // Output of TestRayOBBIntersection()
        glm::vec3 aabb_min(-1.0f, -1.0f, -1.0f);
        glm::vec3 aabb_max(1.0f, 1.0f, 1.0f);

        if (TestRayOBBIntersection(
                ray_origin,
                ray_direction,
                aabb_min,
                aabb_max,
                anchor.modelTransformation,
                intersection_distance)
                ) {
            LOGE("Model selected at position %d", i);
            return;
        }
    }*/

    if (arFrame != nullptr && session != nullptr) {
        ArHitResultList *hit_result_list = nullptr;
        ArHitResultList_create(session, &hit_result_list);
        // CHECK(hit_result_list);
        if (false) {
            ArFrame_hitTestInstantPlacement(session, arFrame, x, y, 2.0, hit_result_list);
        } else {
            ArFrame_hitTest(session, arFrame, x, y, hit_result_list);
        }

        int32_t hit_result_list_size = 0;
        ArHitResultList_getSize(session, hit_result_list, &hit_result_list_size);

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
            glm::mat4 model(1.0);
            GetTransformMatrixFromAnchor(*anchor, session, &model);
            colored_anchor.modelTransformation = model;

            // UpdateAnchorColor(&colored_anchor);
            anchors_.push_back(colored_anchor);

            ArHitResult_destroy(ar_hit_result);
            ar_hit_result = nullptr;

            ArHitResultList_destroy(hit_result_list);
            hit_result_list = nullptr;
        }
    }
}

void ArApplication::onMove(float x, float y) {
    bool isAnchorSelected = false;
    for (auto &anchor : anchors_) {
        if (anchor.currentlySelected) {
            isAnchorSelected = true;
            break;
        }
    }
    if (!isAnchorSelected) {
        // If no anchor selected then do not find hit test
        return;
    }

    ArHitResultList *hit_result_list = nullptr;
    ArHitResultList_create(session, &hit_result_list);
    ArFrame_hitTest(session, arFrame, x, y, hit_result_list);

    int32_t hit_result_list_size = 0;
    ArHitResultList_getSize(session, hit_result_list, &hit_result_list_size);

    glm::mat4 matrix(1.0);
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
            ArPose_destroy(hit_pose);
            if (!in_polygon /*|| normal_distance_to_plane < 0*/) {
                continue;
            }
            ar_hit_result = ar_hit;
            break;
        }
    }

    if (ar_hit_result) {
        ArAnchor *anchor = nullptr;
        if (ArHitResult_acquireNewAnchor(session, ar_hit_result, &anchor) !=
            AR_SUCCESS) {
            return;
        }
        // Get matrix from created anchor and release it
        GetTransformMatrixFromAnchor(*anchor, session, &matrix);

        ArAnchor_release(anchor);
        ArHitResult_destroy(ar_hit_result);
        ar_hit_result = nullptr;
        ArHitResultList_destroy(hit_result_list);
        hit_result_list = nullptr;
    }

    for (auto &anchor : anchors_) {
        // Update matrix from last created anchor
        if (anchor.currentlySelected) {
            anchor.modelTransformation = matrix;
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

void ArApplication::GetTransformMatrixFromAnchor(const ArAnchor &ar_anchor,
                                                 ArSession *ar_session,
                                                 glm::mat4 *out_model_mat) {
    if (out_model_mat == nullptr) {
        LOGE("util::GetTransformMatrixFromAnchor model_mat is null.");
        return;
    }
    ScopedArPose pose(ar_session);
    ArAnchor_getPose(ar_session, &ar_anchor, pose.GetArPose());
    ArPose_getMatrix(ar_session, pose.GetArPose(), glm::value_ptr(*out_model_mat));
}

void ArApplication::ScreenPosToWorldRay(int mouseX, int mouseY, int screenWidth, int screenHeight, glm::mat4 ViewMatrix, glm::mat4 ProjectionMatrix, glm::vec3 &out_origin, glm::vec3 &out_direction) {
    // The ray Start and End positions, in Normalized Device Coordinates (Have you read Tutorial 4 ?)
    glm::vec4 lRayStart_NDC(
            ((float) mouseX / (float) screenWidth - 0.5f) * 2.0f, // [0,1024] -> [-1,1]
            ((float) mouseY / (float) screenHeight - 0.5f) * 2.0f, // [0, 768] -> [-1,1]
            -1.0, // The near plane maps to Z=-1 in Normalized Device Coordinates
            1.0f
    );
    glm::vec4 lRayEnd_NDC(
            ((float) mouseX / (float) screenWidth - 0.5f) * 2.0f,
            ((float) mouseY / (float) screenHeight - 0.5f) * 2.0f,
            0.0,
            1.0f
    );

    // The Projection matrix goes from Camera Space to NDC.
    // So inverse(ProjectionMatrix) goes from NDC to Camera Space.
    glm::mat4 InverseProjectionMatrix = glm::inverse(ProjectionMatrix);

    // The View Matrix goes from World Space to Camera Space.
    // So inverse(ViewMatrix) goes from Camera Space to World Space.
    glm::mat4 InverseViewMatrix = glm::inverse(ViewMatrix);

    glm::vec4 lRayStart_camera = InverseProjectionMatrix * lRayStart_NDC;
    lRayStart_camera /= lRayStart_camera.w;
    glm::vec4 lRayStart_world = InverseViewMatrix * lRayStart_camera;
    lRayStart_world /= lRayStart_world.w;
    glm::vec4 lRayEnd_camera = InverseProjectionMatrix * lRayEnd_NDC;
    lRayEnd_camera /= lRayEnd_camera.w;
    glm::vec4 lRayEnd_world = InverseViewMatrix * lRayEnd_camera;
    lRayEnd_world /= lRayEnd_world.w;


    // Faster way (just one inverse)
    //glm::mat4 M = glm::inverse(ProjectionMatrix * ViewMatrix);
    //glm::vec4 lRayStart_world = M * lRayStart_NDC; lRayStart_world/=lRayStart_world.w;
    //glm::vec4 lRayEnd_world   = M * lRayEnd_NDC  ; lRayEnd_world  /=lRayEnd_world.w;

    glm::vec3 lRayDir_world(lRayEnd_world - lRayStart_world);
    lRayDir_world = glm::normalize(lRayDir_world);

    out_origin = glm::vec3(lRayStart_world);
    out_direction = glm::normalize(lRayDir_world);

}

bool ArApplication::TestRayOBBIntersection(glm::vec3 ray_origin, glm::vec3 ray_direction, glm::vec3 aabb_min, glm::vec3 aabb_max, glm::mat4 ModelMatrix, float &intersection_distance) {
    // Intersection method from Real-Time Rendering and Essential Mathematics for Games

    float tMin = 0.0f;
    float tMax = 100000.0f;

    glm::vec3 OBBposition_worldspace(ModelMatrix[3].x, ModelMatrix[3].y, ModelMatrix[3].z);

    glm::vec3 delta = OBBposition_worldspace - ray_origin;

    // Test intersection with the 2 planes perpendicular to the OBB's X axis
    {
        glm::vec3 xaxis(ModelMatrix[0].x, ModelMatrix[0].y, ModelMatrix[0].z);
        float e = glm::dot(xaxis, delta);
        float f = glm::dot(ray_direction, xaxis);

        if (fabs(f) > 0.001f) { // Standard case

            float t1 = (e + aabb_min.x) / f; // Intersection with the "left" plane
            float t2 = (e + aabb_max.x) / f; // Intersection with the "right" plane
            // t1 and t2 now contain distances betwen ray origin and ray-plane intersections

            // We want t1 to represent the nearest intersection,
            // so if it's not the case, invert t1 and t2
            if (t1 > t2) {
                float w = t1;
                t1 = t2;
                t2 = w; // swap t1 and t2
            }

            // tMax is the nearest "far" intersection (amongst the X,Y and Z planes pairs)
            if (t2 < tMax)
                tMax = t2;
            // tMin is the farthest "near" intersection (amongst the X,Y and Z planes pairs)
            if (t1 > tMin)
                tMin = t1;

            // And here's the trick :
            // If "far" is closer than "near", then there is NO intersection.
            // See the images in the tutorials for the visual explanation.
            if (tMax < tMin)
                return false;

        } else { // Rare case : the ray is almost parallel to the planes, so they don't have any "intersection"
            if (-e + aabb_min.x > 0.0f || -e + aabb_max.x < 0.0f)
                return false;
        }
    }


    // Test intersection with the 2 planes perpendicular to the OBB's Y axis
    // Exactly the same thing than above.
    {
        glm::vec3 yaxis(ModelMatrix[1].x, ModelMatrix[1].y, ModelMatrix[1].z);
        float e = glm::dot(yaxis, delta);
        float f = glm::dot(ray_direction, yaxis);

        if (fabs(f) > 0.001f) {

            float t1 = (e + aabb_min.y) / f;
            float t2 = (e + aabb_max.y) / f;

            if (t1 > t2) {
                float w = t1;
                t1 = t2;
                t2 = w;
            }

            if (t2 < tMax)
                tMax = t2;
            if (t1 > tMin)
                tMin = t1;
            if (tMin > tMax)
                return false;

        } else {
            if (-e + aabb_min.y > 0.0f || -e + aabb_max.y < 0.0f)
                return false;
        }
    }


    // Test intersection with the 2 planes perpendicular to the OBB's Z axis
    // Exactly the same thing than above.
    {
        glm::vec3 zaxis(ModelMatrix[2].x, ModelMatrix[2].y, ModelMatrix[2].z);
        float e = glm::dot(zaxis, delta);
        float f = glm::dot(ray_direction, zaxis);

        if (fabs(f) > 0.001f) {

            float t1 = (e + aabb_min.z) / f;
            float t2 = (e + aabb_max.z) / f;

            if (t1 > t2) {
                float w = t1;
                t1 = t2;
                t2 = w;
            }

            if (t2 < tMax)
                tMax = t2;
            if (t1 > tMin)
                tMin = t1;
            if (tMin > tMax)
                return false;

        } else {
            if (-e + aabb_min.z > 0.0f || -e + aabb_max.z < 0.0f)
                return false;
        }
    }
    intersection_distance = tMin;
    return true;
}