//
// Created by Roshan on 23/04/22.
//

#ifndef OPENGL_AR_H
#define OPENGL_AR_H

#include "GLES3/gl32.h"
#include <android/asset_manager.h>
#include "../../../../../arcore/include/arcore_c_api.h"
#include "BobLampCleanRenderer.h"
#include "background_renderer.h"
#include "Circle.h"
#include "plane_renderer.h"
#include "DepthTexture.h"
#include <Renderer.h>
#include <vector>

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

    void onMove(float x, float y);

private:
    AAssetManager *assetManager;
    ArSession *session = nullptr;
    ArFrame *arFrame = nullptr;
    int32_t width = 1;
    int32_t height = 1;
    int32_t displayRotation = 1;
    bool calculate_uv_transform_ = false;
    int32_t plane_count_ = 0;

    BackgroundRenderer backgroundRenderer;
    PlaneRenderer planeRenderer;
    BobLampCleanRenderer bobLampCleanRenderer;
    Circle circleRenderer;
    DepthTexture depthTexture;

    // The anchors at which we are drawing android models using given colors.
    struct ColoredAnchor {
        ArAnchor *anchor;
        ArTrackable *trackable;
        glm::mat4 modelTransformation;
        bool currentlySelected = false;
        float color[4];
    };

    std::vector<ColoredAnchor> anchors_;

    void configureSession();

    glm::mat3 GetTextureTransformMatrix(const ArSession *session, const ArFrame *frame);

//    glm::vec3 GetPlaneNormal(const ArSession& ar_session,
//                             const ArPose& plane_pose) {
//        float plane_pose_raw[7] = {0.f};
//        ArPose_getPoseRaw(&ar_session, &plane_pose, plane_pose_raw);
//        glm::quat plane_quaternion(plane_pose_raw[3], plane_pose_raw[0],
//                                   plane_pose_raw[1], plane_pose_raw[2]);
//        // Get normal vector, normal is defined to be positive Y-position in local
//        // frame.
//        return glm::rotate(plane_quaternion, glm::vec3(0., 1.f, 0.));
//    }
//
//    float CalculateDistanceToPlane(const ArSession& ar_session,
//                                   const ArPose& plane_pose,
//                                   const ArPose& camera_pose) {
//        float plane_pose_raw[7] = {0.f};
//        ArPose_getPoseRaw(&ar_session, &plane_pose, plane_pose_raw);
//        glm::vec3 plane_position(plane_pose_raw[4], plane_pose_raw[5],
//                                 plane_pose_raw[6]);
//        glm::vec3 normal = GetPlaneNormal(ar_session, plane_pose);
//
//        float camera_pose_raw[7] = {0.f};
//        ArPose_getPoseRaw(&ar_session, &camera_pose, camera_pose_raw);
//        glm::vec3 camera_P_plane(camera_pose_raw[4] - plane_position.x,
//                                 camera_pose_raw[5] - plane_position.y,
//                                 camera_pose_raw[6] - plane_position.z);
//        return glm::dot(normal, camera_P_plane);
//    }
    void GetTransformMatrixFromAnchor(const ArAnchor &ar_anchor, ArSession *ar_session,
                                      glm::mat4 *out_model_mat);
};

#endif //OPENGL_AR_H
