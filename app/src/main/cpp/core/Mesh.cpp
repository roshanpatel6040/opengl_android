//
// Created by Roshan on 19/12/21.
//

#include <cassert>
#include <android/log.h>
#include "Mesh.h"

Mesh::MeshEntry::MeshEntry() {
    VB = -1;
    IB = -1;
    NumIndices = 0;
    MaterialIndex = INVALID_MATERIAL;
};

Mesh::MeshEntry::~MeshEntry() {
    if (VB != -1) {
        glDeleteBuffers(1, &VB);
    }

    if (IB != -1) {
        glDeleteBuffers(1, &IB);
    }
}

void Mesh::MeshEntry::Init(const std::vector<Vertex> &Vertices,
                           const std::vector<unsigned int> &Indices) {
    NumIndices = Indices.size();

    glGenBuffers(1, &VB);
    glBindBuffer(GL_ARRAY_BUFFER, VB);
    glBufferData(GL_ARRAY_BUFFER, sizeof(Vertex) * Vertices.size(), &Vertices[0], GL_STATIC_DRAW);

    glGenBuffers(1, &IB);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, IB);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(unsigned int) * NumIndices, &Indices[0],
                 GL_STATIC_DRAW);
}

Mesh::Mesh() {
}

Mesh::Mesh(AAssetManager *manager) : assetManager(manager) {
}

Mesh::~Mesh() {
    Clear();
}


void Mesh::Clear() {
    for (unsigned int i = 0; i < m_Textures.size(); i++) {
        m_Textures[i]->unBind();
        m_Textures[i]->~Texture();
    }
    m_Textures.clear();
}


bool Mesh::LoadMesh(const std::string &Filename) {
    // Release the previously loaded mesh (if it exists)
    Clear();

    bool Ret = false;
    Assimp::AndroidJNIIOSystem *ioSystem = new Assimp::AndroidJNIIOSystem(
            "storage/emulated/0/Android/data/com.demo.opengl/files/models", assetManager);
    Assimp::Importer *Importer = new Assimp::Importer();
    bool isFileExist = ioSystem->Exists(Filename.c_str());
    __android_log_print(ANDROID_LOG_DEBUG, "Model loading", "File exits: %d",
                        isFileExist);
    Importer->SetIOHandler(ioSystem);
//    aiProcess_Triangulate | aiProcess_GenSmoothNormals | aiProcess_FlipUVs | aiProcess_JoinIdenticalVertices
    const aiScene *pScene = Importer->ReadFile(Filename.c_str(),
                                               aiProcessPreset_TargetRealtime_MaxQuality);

    if (pScene) {
        Ret = InitFromScene(pScene, Filename);
    } else {
        __android_log_print(ANDROID_LOG_ERROR, "Model Loading", "Filename: %s, Error: %s",
                            Filename.c_str(), Importer->GetErrorString());
    }

    return Ret;
}

bool Mesh::InitFromScene(const aiScene *pScene, const std::string &Filename) {
    m_Entries.resize(pScene->mNumMeshes);
    m_Textures.resize(pScene->mNumMaterials);

    // Initialize the meshes in the scene one by one
    for (unsigned int i = 0; i < m_Entries.size(); i++) {
        const aiMesh *paiMesh = pScene->mMeshes[i];
        InitMesh(i, paiMesh);
    }

    return InitMaterials(pScene, Filename);
}

void Mesh::InitMesh(unsigned int Index, const aiMesh *paiMesh) {
    m_Entries[Index].MaterialIndex = paiMesh->mMaterialIndex;

    std::vector<Vertex> Vertices;
    std::vector<unsigned int> Indices;

    const aiVector3D Zero3D(0.0f, 0.0f, 0.0f);

    for (unsigned int i = 0; i < paiMesh->mNumVertices; i++) {
        const aiVector3D *pPos = &(paiMesh->mVertices[i]);
        const aiVector3D *pNormal = &(paiMesh->mNormals[i]);
        const aiVector3D *pTexCoord = paiMesh->HasTextureCoords(0)
                                      ? &(paiMesh->mTextureCoords[0][i]) : &Zero3D;

        Vertex v(Vector3f(pPos->x, pPos->y, pPos->z),
                 Vector2f(pTexCoord->x, pTexCoord->y),
                 Vector3f(pNormal->x, pNormal->y, pNormal->z));

        Vertices.push_back(v);
    }

    for (unsigned int i = 0; i < paiMesh->mNumFaces; i++) {
        const aiFace &Face = paiMesh->mFaces[i];
        assert(Face.mNumIndices == 3);
        Indices.push_back(Face.mIndices[0]);
        Indices.push_back(Face.mIndices[1]);
        Indices.push_back(Face.mIndices[2]);
    }

    m_Entries[Index].Init(Vertices, Indices);
}

bool Mesh::InitMaterials(const aiScene *pScene, const std::string &Filename) {
    // Extract the directory part from the file name
    std::string::size_type SlashIndex = Filename.find_last_of('/');
    std::string Dir;

    if (SlashIndex == std::string::npos) {
        Dir = ".";
    } else if (SlashIndex == 0) {
        Dir = "/";
    } else {
        Dir = Filename.substr(0, SlashIndex);
    }

    bool Ret = true;

    // Initialize the materials
    for (unsigned int i = 0; i < pScene->mNumMaterials; i++) {
        const aiMaterial *pMaterial = pScene->mMaterials[i];

        m_Textures[i] = nullptr;

        if (pMaterial->GetTextureCount(aiTextureType_DIFFUSE) > 0) {
            aiString Path;

            if (pMaterial->GetTexture(aiTextureType_DIFFUSE, 0, &Path,
                                      nullptr,
                                      nullptr,
                                      nullptr,
                                      nullptr,
                                      nullptr) == AI_SUCCESS) {
                std::string FullPath = Dir + "/" + Path.data;
                m_Textures[i] = new Texture(GL_TEXTURE_2D, FullPath);

                if (!m_Textures[i]->load()) {
                    __android_log_print(ANDROID_LOG_ERROR, "Model loading", "Texture not loaded");
                    delete m_Textures[i];
                    m_Textures[i] = nullptr;
                    Ret = false;
                } else {
                    __android_log_print(ANDROID_LOG_DEBUG, "Model loading", "Texture loaded");
                }
            }
        }

        // Load a white texture in case the model does not include its own texture
        if (!m_Textures[i]) {
            // TODO change path to default texture png
            m_Textures[i] = new Texture(GL_TEXTURE_2D, "storage/emulated/0/texture.png");
            Ret = m_Textures[i]->load();
        }
    }

    return Ret;
}

void Mesh::Render() {
    glEnableVertexAttribArray(0);
    glEnableVertexAttribArray(1);
    glEnableVertexAttribArray(2);

    for (unsigned int i = 0; i < m_Entries.size(); i++) {
        glBindBuffer(GL_ARRAY_BUFFER, m_Entries[i].VB);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), nullptr);
        glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (const GLvoid *) 12);
        glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (const GLvoid *) 20);

        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_Entries[i].IB);

        const unsigned int MaterialIndex = m_Entries[i].MaterialIndex;

        if (MaterialIndex < m_Textures.size() && m_Textures[MaterialIndex]) {
            m_Textures[MaterialIndex]->bind();
        }

        glDrawElements(GL_TRIANGLES, m_Entries[i].NumIndices, GL_UNSIGNED_INT, nullptr);
    }

    glDisableVertexAttribArray(0);
    glDisableVertexAttribArray(1);
    glDisableVertexAttribArray(2);
}