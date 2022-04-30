#ifndef C_ARCORE_HELLOE_AR_PLANE_RENDERER_H_
#define C_ARCORE_HELLOE_AR_PLANE_RENDERER_H_

#include <GLES2/gl2.h>
#include <GLES2/gl2ext.h>
#include <android/asset_manager.h>
#include <array>
#include <cstdint>
#include <cstdlib>
#include <vector>
#include <glm.hpp>
#include <ext.hpp>
#include "gtx/quaternion.hpp"
#include "../../../arcore/include/arcore_c_api.h"
#include <string>
#include "Shader.h"

class ScopedArPose {
public:
    explicit ScopedArPose(const ArSession *session) {
        ArPose_create(session, nullptr, &pose_);
    }

    ~ScopedArPose() { ArPose_destroy(pose_); }

    ArPose *GetArPose() { return pose_; }

    // Delete copy constructors.
    ScopedArPose(const ScopedArPose &) = delete;

    void operator=(const ScopedArPose &) = delete;

private:
    ArPose *pose_;
};

class PlaneRenderer {
public:
    PlaneRenderer() = default;

    ~PlaneRenderer() = default;

    // Sets up OpenGL state used by the plane renderer.  Must be called on the
    // OpenGL thread.
    void InitializeGlContent(AAssetManager *asset_manager);

    // Draws the provided plane.
    void Draw(const glm::mat4 &projection_mat, const glm::mat4 &view_mat,
              const ArSession &ar_session, const ArPlane &ar_plane);

private:
    void UpdateForPlane(const ArSession &ar_session, const ArPlane &ar_plane);

    std::vector<glm::vec3> vertices_;
    std::vector<GLushort> triangles_;
    glm::mat4 model_mat_ = glm::mat4(1.0f);
    glm::vec3 normal_vec_ = glm::vec3(0.0f);

    GLuint texture_id_;

    GLuint shader_program_;
    GLint attri_vertices_;
    GLint uniform_mvp_mat_;
    GLint uniform_texture_;
    GLint uniform_model_mat_;
    GLint uniform_normal_vec_;

    Shader *shader;

    glm::vec3 GetPlaneNormal(const ArSession &ar_session, const ArPose &plane_pose);

    bool LoadTextFileFromAssetManager(const char *file_name, AAssetManager *asset_manager,
                                      std::string *out_file_text_string);
};

#endif  // C_ARCORE_HELLOE_AR_PLANE_RENDERER_H_
