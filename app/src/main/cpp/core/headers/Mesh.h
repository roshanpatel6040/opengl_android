//
// Created by Roshan on 19/12/21.
//

#ifndef OPENGL_MESH_H
#define OPENGL_MESH_H

#define TIME

#include <iostream>
#include "GLES2/gl2.h"
#include "assimp/Importer.hpp"  // C++ importer interface
#include <assimp/scene.h>       // Output data structure
#include <assimp/postprocess.h> // Post processing flags
#include <assimp/port/AndroidJNI/AndroidJNIIOSystem.h>
#include "Texture.h"
#include "Utils.h"
#include "WorldTransform.h"
#include <vector>
#include <map>
#include <Material.h>
#include <sys/time.h>


#define ASSIMP_LOAD_FLAGS (aiProcess_Triangulate | aiProcess_GenSmoothNormals |  aiProcess_CalcTangentSpace | aiProcess_FlipUVs)

#define POSITION_LOCATION    0
#define TEX_COORD_LOCATION   1
#define NORMAL_LOCATION      2
#define BONE_ID_LOCATION     3
#define BONE_WEIGHT_LOCATION 4

class Mesh {

public:
    Mesh();

    Mesh(AAssetManager *manager);

    ~Mesh();

    bool LoadMesh(const std::string &Filename);

    void Render(GLuint program);

    uint NumBones() const {
        return (uint) m_BoneNameToIndexMap.size();
    }

    WorldTrans &GetWorldTransform() { return m_worldTransform; }

    const Material &GetMaterial();

    void GetBoneTransforms(float AnimationTimeSec, std::vector<Matrix4f> &Transforms);

    float getAnimationSecond() const;

    long long currentTimeInMilliseconds() const;

#define INVALID_MATERIAL 0xFFFFFFFF

    struct BasicMeshEntry {
        BasicMeshEntry() {
            NumIndices = 0;
            BaseVertex = 0;
            BaseIndex = 0;
            MaterialIndex = INVALID_MATERIAL;
        }

        unsigned int NumIndices;
        unsigned int BaseVertex;
        unsigned int BaseIndex;
        unsigned int MaterialIndex;
    };

    std::vector<BasicMeshEntry> m_Meshes;
    std::vector<Material> m_Materials;

private:

    AAssetManager *assetManager;

    long long startTimeInMillis;

    void Clear();

    bool InitFromScene(const aiScene *pScene, const std::string &Filename);

    void CountVerticesAndIndices(const aiScene *pScene, unsigned int &NumVertices,
                                 unsigned int &NumIndices);

    void ReserveSpace(unsigned int NumVertices, unsigned int NumIndices);

    void InitAllMeshes(const aiScene *pScene);

    void InitSingleMesh(uint MeshIndex, const aiMesh *paiMesh);

    bool InitMaterials(const aiScene *pScene, const std::string &Filename);

    void PopulateBuffers();

    void LoadTextures(const std::string &Dir, const aiMaterial *pMaterial, int index);

    void LoadDiffuseTexture(const std::string &Dir, const aiMaterial *pMaterial, int index);

    void LoadSpecularTexture(const std::string &Dir, const aiMaterial *pMaterial, int index);

    void LoadColors(const aiMaterial *pMaterial, int index);

    struct VertexBoneData {
        uint BoneIDs[MAX_NUM_BONES_PER_VERTEX] = {0};
        float Weights[MAX_NUM_BONES_PER_VERTEX] = {0.0f};

        VertexBoneData() {
        }

        void AddBoneData(uint BoneID, float Weight) {
            for (uint i = 0; i < ARRAY_SIZE_IN_ELEMENTS(BoneIDs); i++) {
                if (Weights[i] == 0.0) {
                    BoneIDs[i] = BoneID;
                    Weights[i] = Weight;
                    //printf("Adding bone %d weight %f at index %i\n", BoneID, Weight, i);
                    return;
                }
            }

            // should never get here - more bones than we have space for
            // assert(0);
        }
    };

    void LoadMeshBones(uint MeshIndex, const aiMesh *paiMesh);

    void LoadSingleBone(uint MeshIndex, const aiBone *pBone);

    int GetBoneId(const aiBone *pBone);

    void CalcInterpolatedScaling(aiVector3D &Out, float AnimationTime, const aiNodeAnim *pNodeAnim);

    void
    CalcInterpolatedRotation(aiQuaternion &Out, float AnimationTime, const aiNodeAnim *pNodeAnim);

    void
    CalcInterpolatedPosition(aiVector3D &Out, float AnimationTime, const aiNodeAnim *pNodeAnim);

    uint FindScaling(float AnimationTime, const aiNodeAnim *pNodeAnim);

    uint FindRotation(float AnimationTime, const aiNodeAnim *pNodeAnim);

    uint FindPosition(float AnimationTime, const aiNodeAnim *pNodeAnim);

    const aiNodeAnim *FindNodeAnim(const aiAnimation *pAnimation, const std::string &NodeName);

    void
    ReadNodeHierarchy(float AnimationTime, const aiNode *pNode, const Matrix4f &ParentTransform);

#define INVALID_MATERIAL 0xFFFFFFFF

    enum BUFFER_TYPE {
        INDEX_BUFFER = 0,
        POS_VB = 1,
        TEXCOORD_VB = 2,
        NORMAL_VB = 3,
        BONE_VB = 4,
        NUM_BUFFERS = 5
    };

    WorldTrans m_worldTransform;
    GLuint m_VAO = 0;
    GLuint m_Buffers[NUM_BUFFERS] = {0};

    Assimp::Importer Importer;
    const aiScene *pScene = NULL;

    // Temporary space for vertex stuff before we load them into the GPU
    std::vector<Vector3f> m_Positions;
    std::vector<Vector3f> m_Normals;
    std::vector<Vector2f> m_TexCoords;
    std::vector<unsigned int> m_Indices;
    std::vector<VertexBoneData> m_Bones;

    std::map<std::string, uint> m_BoneNameToIndexMap;

    struct BoneInfo {
        Matrix4f OffsetMatrix;
        Matrix4f FinalTransformation;

        BoneInfo(const Matrix4f &Offset) {
            OffsetMatrix = Offset;
            FinalTransformation.SetZero();
        }
    };

    std::vector<BoneInfo> m_BoneInfo;
    Matrix4f m_GlobalInverseTransform;

};

#endif //OPENGL_MESH_H
