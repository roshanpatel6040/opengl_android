//
// Created by Roshan on 19/12/21.
//

#ifndef OPENGL_MESH_H
#define OPENGL_MESH_H

#include <iostream>
#include "GLES2/gl2.h"
#include "assimp/Importer.hpp"  // C++ importer interface
#include <assimp/scene.h>       // Output data structure
#include <assimp/postprocess.h> // Post processing flags
#include <assimp/port/AndroidJNI/AndroidJNIIOSystem.h>
#include "Utils.h"
#include "Texture.h"
#include <vector>

struct Vertex {
    Vector3f m_pos;
    Vector2f m_tex;
    Vector3f m_normal;

    Vertex() {}

    Vertex(const Vector3f &pos, const Vector2f &tex, const Vector3f &normal) {
        m_pos = pos;
        m_tex = tex;
        m_normal = normal;
    }

    Vertex(const Vector3f &pos, const Vector2f &tex) {
        m_pos = pos;
        m_tex = tex;
        m_normal = Vector3f(0.0f, 0.0f, 0.0f);
    }
};

class Mesh {
public:
    Mesh();

    Mesh(AAssetManager *manager);

    ~Mesh();

    bool LoadMesh(const std::string &Filename);

    void Render();

#define INVALID_MATERIAL 0xFFFFFFFF

    struct MeshEntry {
        MeshEntry();

        ~MeshEntry();

        void Init(const std::vector<Vertex> &Vertices,
                  const std::vector<unsigned int> &Indices);

        GLuint VB;
        GLuint IB;
        unsigned int NumIndices;
        unsigned int MaterialIndex;
    };

    std::vector<MeshEntry> m_Entries;
    std::vector<Texture *> m_Textures;

private:

    AAssetManager *assetManager;

    bool InitFromScene(const aiScene *pScene, const std::string &Filename);

    void InitMesh(unsigned int Index, const aiMesh *paiMesh);

    bool InitMaterials(const aiScene *pScene, const std::string &Filename);

    void Clear();
};

#endif //OPENGL_MESH_H
