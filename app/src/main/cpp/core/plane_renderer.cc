#include "plane_renderer.h"

constexpr char kVertexShaderFilename[] = "shaders/plane.vert";
constexpr char kFragmentShaderFilename[] = "shaders/plane.frag";

void PlaneRenderer::InitializeGlContent(AAssetManager *asset_manager) {
    shader_program_ = glCreateProgram();
    std::string vertexSource;
    LoadTextFileFromAssetManager(kVertexShaderFilename, asset_manager, &vertexSource);
    std::string fragmentSource;
    LoadTextFileFromAssetManager(kFragmentShaderFilename, asset_manager, &fragmentSource);
    shader = new Shader(shader_program_, vertexSource, fragmentSource);
    glLinkProgram(shader_program_);
    if (!shader_program_) {
        // LOGE("Could not create program.");
    }

    uniform_mvp_mat_ = glGetUniformLocation(shader_program_, "mvp");
    uniform_texture_ = glGetUniformLocation(shader_program_, "texture");
    uniform_model_mat_ = glGetUniformLocation(shader_program_, "model_mat");
    uniform_normal_vec_ = glGetUniformLocation(shader_program_, "normal");
    attri_vertices_ = glGetAttribLocation(shader_program_, "vertex");

//    glGenTextures(1, &texture_id_);
//    glBindTexture(GL_TEXTURE_2D, texture_id_);
//    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
//    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
//    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER,
//                    GL_LINEAR_MIPMAP_LINEAR);
//    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

//    if (!util::LoadPngFromAssetManager(GL_TEXTURE_2D, "models/trigrid.png")) {
//        // LOGE("Could not load png texture for planes.");
//    }

    glGenerateMipmap(GL_TEXTURE_2D);

    glBindTexture(GL_TEXTURE_2D, 0);
}

void PlaneRenderer::Draw(const glm::mat4 &projection_mat,
                         const glm::mat4 &view_mat, const ArSession &ar_session,
                         const ArPlane &ar_plane) {
    if (!shader_program_) {
        // LOGE("shader_program is null.");
        return;
    }

    UpdateForPlane(ar_session, ar_plane);

    glUseProgram(shader_program_);
    glDepthMask(GL_FALSE);

//    glActiveTexture(GL_TEXTURE0);
//    glUniform1i(uniform_texture_, 0);
//    glBindTexture(GL_TEXTURE_2D, texture_id_);

    // Compose final mvp matrix for this plane renderer.
    glUniformMatrix4fv(uniform_mvp_mat_, 1, GL_FALSE,
                       glm::value_ptr(projection_mat * view_mat * model_mat_));

    glUniformMatrix4fv(uniform_model_mat_, 1, GL_FALSE,
                       glm::value_ptr(model_mat_));
    glUniform3f(uniform_normal_vec_, normal_vec_.x, normal_vec_.y, normal_vec_.z);

    glEnableVertexAttribArray(attri_vertices_);
    glVertexAttribPointer(attri_vertices_, 3, GL_FLOAT, GL_FALSE, 0,
                          vertices_.data());

    glEnable(GL_BLEND);

    // Textures are loaded with premultiplied alpha
    // (https://developer.android.com/reference/android/graphics/BitmapFactory.Options#inPremultiplied),
    // so we use the premultiplied alpha blend factors.
    glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
    glDrawElements(GL_TRIANGLES, triangles_.size(), GL_UNSIGNED_SHORT,
                   triangles_.data());

    glDisable(GL_BLEND);
    glUseProgram(0);
    glDepthMask(GL_TRUE);
}

void PlaneRenderer::UpdateForPlane(const ArSession &ar_session,
                                   const ArPlane &ar_plane) {
    // The following code generates a triangle mesh filling a convex polygon,
    // including a feathered edge for blending.
    //
    // The indices shown in the diagram are used in comments below.
    // _______________     0_______________1
    // |             |      |4___________5|
    // |             |      | |         | |
    // |             | =>   | |         | |
    // |             |      | |         | |
    // |             |      |7-----------6|
    // ---------------     3---------------2

    vertices_.clear();
    triangles_.clear();

    int32_t polygon_length;
    ArPlane_getPolygonSize(&ar_session, &ar_plane, &polygon_length);

    if (polygon_length == 0) {
        // LOGE("PlaneRenderer::UpdatePlane, no valid plane polygon is found");
        return;
    }

    const int32_t vertices_size = polygon_length / 2;
    std::vector<glm::vec2> raw_vertices(vertices_size);
    ArPlane_getPolygon(&ar_session, &ar_plane,
                       glm::value_ptr(raw_vertices.front()));

    // Fill vertex 0 to 3. Note that the vertex.xy are used for x and z
    // position. vertex.z is used for alpha. The outer polygon's alpha
    // is 0.
    for (int32_t i = 0; i < vertices_size; ++i) {
        vertices_.push_back(glm::vec3(raw_vertices[i].x, raw_vertices[i].y, 0.0f));
    }

    ScopedArPose scopedArPose(&ar_session);
    ArPlane_getCenterPose(&ar_session, &ar_plane, scopedArPose.GetArPose());
    ArPose_getMatrix(&ar_session, scopedArPose.GetArPose(),
                     glm::value_ptr(model_mat_));
    normal_vec_ = GetPlaneNormal(ar_session, *scopedArPose.GetArPose());

    // Feather distance 0.2 meters.
    const float kFeatherLength = 0.2f;
    // Feather scale over the distance between plane center and vertices.
    const float kFeatherScale = 0.2f;

    // Fill vertex 4 to 7, with alpha set to 1.
    for (int32_t i = 0; i < vertices_size; ++i) {
        // Vector from plane center to current point.
        glm::vec2 v = raw_vertices[i];
        const float scale =
                1.0f - std::min((kFeatherLength / glm::length(v)), kFeatherScale);
        const glm::vec2 result_v = scale * v;

        vertices_.push_back(glm::vec3(result_v.x, result_v.y, 1.0f));
    }

    const int32_t vertices_length = vertices_.size();
    const int32_t half_vertices_length = vertices_length / 2;

    // Generate triangle (4, 5, 6) and (4, 6, 7).
    for (int i = half_vertices_length + 1; i < vertices_length - 1; ++i) {
        triangles_.push_back(half_vertices_length);
        triangles_.push_back(i);
        triangles_.push_back(i + 1);
    }

    // Generate triangle (0, 1, 4), (4, 1, 5), (5, 1, 2), (5, 2, 6),
    // (6, 2, 3), (6, 3, 7), (7, 3, 0), (7, 0, 4)
    for (int i = 0; i < half_vertices_length; ++i) {
        triangles_.push_back(i);
        triangles_.push_back((i + 1) % half_vertices_length);
        triangles_.push_back(i + half_vertices_length);

        triangles_.push_back(i + half_vertices_length);
        triangles_.push_back((i + 1) % half_vertices_length);
        triangles_.push_back((i + half_vertices_length + 1) % half_vertices_length +
                             half_vertices_length);
    }
}

glm::vec3 PlaneRenderer::GetPlaneNormal(const ArSession &ar_session,
                                        const ArPose &plane_pose) {
    float plane_pose_raw[7] = {0.f};
    ArPose_getPoseRaw(&ar_session, &plane_pose, plane_pose_raw);
    glm::quat plane_quaternion(plane_pose_raw[3], plane_pose_raw[0],
                               plane_pose_raw[1], plane_pose_raw[2]);
    // Get normal vector, normal is defined to be positive Y-position in local
    // frame.
    return glm::rotate(plane_quaternion, glm::vec3(0., 1.f, 0.));
}

bool PlaneRenderer::LoadTextFileFromAssetManager(const char *file_name,
                                                 AAssetManager *asset_manager,
                                                 std::string *out_file_text_string) {
    // If the file hasn't been uncompressed, load it to the internal storage.
    // Note that AAsset_openFileDescriptor doesn't support compressed
    // files (.obj).
    AAsset *asset =
            AAssetManager_open(asset_manager, file_name, AASSET_MODE_STREAMING);
    if (asset == nullptr) {
        // LOGE("Error opening asset %s", file_name);
        return false;
    }

    off_t file_size = AAsset_getLength(asset);
    out_file_text_string->resize(file_size);
    int ret = AAsset_read(asset, &out_file_text_string->front(), file_size);

    if (ret <= 0) {
        // LOGE("Failed to open file: %s", file_name);
        AAsset_close(asset);
        return false;
    }

    AAsset_close(asset);
    return true;
}