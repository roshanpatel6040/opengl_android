//
// Created by Roshan on 19/12/21.
//

#include <cassert>
#include <utility>
#include <android/log.h>
#include "Mesh.h"
#include <GLES2/gl2.h>
#include <GLES3/gl32.h>
#include <Renderer.h>

Mesh::Mesh() {
}

Mesh::Mesh(AAssetManager *manager) : assetManager(manager) {
}

long long Mesh::currentTimeInMilliseconds() const {
    struct timeval tv{};
    gettimeofday(&tv, nullptr);
    return ((tv.tv_sec * 1000) + (tv.tv_usec / 1000));
}

Mesh::~Mesh() {
    Clear();
}

void Mesh::Clear() {
    if (m_Buffers[0] != 0) {
        glDeleteBuffers(ARRAY_SIZE_IN_ELEMENTS(m_Buffers), m_Buffers);
    }

    if (m_VAO != 0) {
        glDeleteVertexArrays(1, &m_VAO);
        m_VAO = 0;
    }
}

bool Mesh::LoadMesh(const std::string &Filename) {
    // Release the previously loaded mesh (if it exists)
    Clear();

    // Create the VAO
    GLCall(glGenVertexArrays(1, &m_VAO))

    GLCall(glBindVertexArray(m_VAO))

    // Create the buffers for the vertices attributes
    glGenBuffers(ARRAY_SIZE_IN_ELEMENTS(m_Buffers), m_Buffers);


    bool Ret = false;
    Assimp::AndroidJNIIOSystem *ioSystem = new Assimp::AndroidJNIIOSystem(
            "storage/emulated/0/Android/data/com.demo.opengl/files/models", assetManager);
    Assimp::Importer *Importer = new Assimp::Importer();
    bool isFileExist = ioSystem->Exists(Filename.c_str());
    __android_log_print(ANDROID_LOG_DEBUG, "Model loading", "File exits: %d",
                        isFileExist);
    Importer->SetIOHandler(ioSystem);
//    aiProcess_Triangulate | aiProcess_GenSmoothNormals | aiProcess_FlipUVs | aiProcess_JoinIdenticalVertices
    pScene = Importer->ReadFile(Filename.c_str(), ASSIMP_LOAD_FLAGS);

    if (pScene) {
        m_GlobalInverseTransform = pScene->mRootNode->mTransformation;
        m_GlobalInverseTransform.Inverse();
        Ret = InitFromScene(pScene, Filename);
    } else {
        __android_log_print(ANDROID_LOG_ERROR, "Model Loading", "Filename: %s, Error: %s",
                            Filename.c_str(), Importer->GetErrorString());
    }

    // Make sure VAO is not changed from outside
    GLCall(glBindVertexArray(0))

    startTimeInMillis = currentTimeInMilliseconds();

    return Ret;
}

bool Mesh::InitFromScene(const aiScene *pScene, const std::string &Filename) {
    m_Meshes.resize(pScene->mNumMeshes);
    m_Materials.resize(pScene->mNumMaterials);

    unsigned int NumVertices = 0;
    unsigned int NumIndices = 0;

    CountVerticesAndIndices(pScene, NumVertices, NumIndices);

    ReserveSpace(NumVertices, NumIndices);

    InitAllMeshes(pScene);

    if (!InitMaterials(pScene, Filename)) {
        return false;
    }
    __android_log_print(ANDROID_LOG_ERROR, "Mesh", "All meshes loaded");

    GLCall(PopulateBuffers())

    return glGetError() == GL_NO_ERROR;
}

void Mesh::CountVerticesAndIndices(const aiScene *pScene, unsigned int &NumVertices,
                                   unsigned int &NumIndices) {
    for (unsigned int i = 0; i < m_Meshes.size(); i++) {
        m_Meshes[i].MaterialIndex = pScene->mMeshes[i]->mMaterialIndex;
        m_Meshes[i].NumIndices = pScene->mMeshes[i]->mNumFaces * 3;
        m_Meshes[i].BaseVertex = NumVertices;
        m_Meshes[i].BaseIndex = NumIndices;

        NumVertices += pScene->mMeshes[i]->mNumVertices;
        NumIndices += m_Meshes[i].NumIndices;
    }
}

void Mesh::ReserveSpace(unsigned int NumVertices, unsigned int NumIndices) {
    m_Positions.reserve(NumVertices);
    m_Normals.reserve(NumVertices);
    m_TexCoords.reserve(NumVertices);
    m_Indices.reserve(NumIndices);
    m_Bones.resize(NumVertices);
}


void Mesh::InitAllMeshes(const aiScene *pScene) {
    for (unsigned int i = 0; i < m_Meshes.size(); i++) {
        const aiMesh *paiMesh = pScene->mMeshes[i];
        InitSingleMesh(i, paiMesh);
    }
}

void Mesh::InitSingleMesh(uint MeshIndex, const aiMesh *paiMesh) {
    const aiVector3D Zero3D(0.0f, 0.0f, 0.0f);
    // Populate the vertex attribute vectors
    for (unsigned int i = 0; i < paiMesh->mNumVertices; i++) {
        const aiVector3D &pPos = paiMesh->mVertices[i];
        Vector3f vertices(pPos.x, pPos.y, pPos.z);
        m_Positions.push_back(vertices);

        if (paiMesh->mNormals) {
            const aiVector3D &pNormal = paiMesh->mNormals[i];
            Vector3f normal(pNormal.x, pNormal.y, pNormal.z);
            m_Normals.push_back(normal);
        } else {
            aiVector3D Normal(0.0f, 1.0f, 0.0f);
            Vector3f normal(Normal.x, Normal.y, Normal.z);
            m_Normals.push_back(normal);
        }
        const aiVector3D &pTexCoord = paiMesh->HasTextureCoords(0) ? paiMesh->mTextureCoords[0][i]
                                                                   : Zero3D;
        Vector2f texChords(pTexCoord.x, pTexCoord.y);
        m_TexCoords.push_back(texChords);
    }

    LoadMeshBones(MeshIndex, paiMesh);

    // Populate the index buffer
    for (unsigned int i = 0; i < paiMesh->mNumFaces; i++) {
        const aiFace &Face = paiMesh->mFaces[i];
        //        printf("num indices %d\n", Face.mNumIndices);
        //        assert(Face.mNumIndices == 3);
        m_Indices.push_back(Face.mIndices[0]);
        m_Indices.push_back(Face.mIndices[1]);
        m_Indices.push_back(Face.mIndices[2]);
    }
}


void Mesh::LoadMeshBones(uint MeshIndex, const aiMesh *pMesh) {
    for (uint i = 0; i < pMesh->mNumBones; i++) {
        LoadSingleBone(MeshIndex, pMesh->mBones[i]);
    }
}


void Mesh::LoadSingleBone(uint MeshIndex, const aiBone *pBone) {
    int BoneId = GetBoneId(pBone);

    if (BoneId == m_BoneInfo.size()) {
        BoneInfo bi(pBone->mOffsetMatrix);
        m_BoneInfo.push_back(bi);
    }

    for (uint i = 0; i < pBone->mNumWeights; i++) {
        const aiVertexWeight &vw = pBone->mWeights[i];
        uint GlobalVertexID = m_Meshes[MeshIndex].BaseVertex + pBone->mWeights[i].mVertexId;
        m_Bones[GlobalVertexID].AddBoneData(BoneId, vw.mWeight);
    }
}


int Mesh::GetBoneId(const aiBone *pBone) {
    int BoneIndex = 0;
    std::string BoneName(pBone->mName.C_Str());

    if (m_BoneNameToIndexMap.find(BoneName) == m_BoneNameToIndexMap.end()) {
        // Allocate an index for a new bone
        BoneIndex = (int) m_BoneNameToIndexMap.size();
        m_BoneNameToIndexMap[BoneName] = BoneIndex;
    } else {
        BoneIndex = m_BoneNameToIndexMap[BoneName];
    }

    return BoneIndex;
}


std::string GetDirFromFilename(const std::string &Filename) {
    std::string::size_type SlashIndex = Filename.find_last_of('/');
    std::string Dir;

    if (SlashIndex == std::string::npos) {
        Dir = ".";
    } else if (SlashIndex == 0) {
        Dir = "/";
    } else {
        Dir = Filename.substr(0, SlashIndex);
    }
    std::string root = "storage/emulated/0/Android/data/com.demo.opengl/files/models/" + Dir;
    return root;
}


bool Mesh::InitMaterials(const aiScene *pScene, const std::string &Filename) {
    std::string Dir = GetDirFromFilename(Filename);

    bool Ret = true;

    __android_log_print(ANDROID_LOG_ERROR, "mesh", "Num materials %d", pScene->mNumMaterials);

    // Initialize the materials
    for (unsigned int i = 0; i < pScene->mNumMaterials; i++) {
        const aiMaterial *pMaterial = pScene->mMaterials[i];

        LoadTextures(Dir, pMaterial, i);

        LoadColors(pMaterial, i);
    }

    return Ret;
}


void Mesh::LoadTextures(const std::string &Dir, const aiMaterial *pMaterial, int index) {
    LoadDiffuseTexture(Dir, pMaterial, index);
    LoadSpecularTexture(Dir, pMaterial, index);
}


void Mesh::LoadDiffuseTexture(const std::string &Dir, const aiMaterial *pMaterial, int index) {
    m_Materials[index].pDiffuse = NULL;

    if (pMaterial->GetTextureCount(aiTextureType_DIFFUSE) > 0) {
        aiString Path;

        if (pMaterial->GetTexture(aiTextureType_DIFFUSE, 0, &Path, NULL, NULL, NULL, NULL, NULL) ==
            AI_SUCCESS) {
            std::string p(Path.data);
            if (p.substr(0, 2) == ".\\") {
                p = p.substr(2, p.size() - 2);
            }
            // replaces incorrect path separator
            std::replace(p.begin(), p.end(), '\\', '/');

            std::string FullPath = Dir + "/" + p;
            __android_log_print(ANDROID_LOG_ERROR, "mesh", "diffuse path %s", FullPath.c_str());
            m_Materials[index].pDiffuse = new Texture(GL_TEXTURE_2D, FullPath.c_str(),
                                                      DIFFUSE_TEXTURE_UNIT);

            if (!m_Materials[index].pDiffuse->load()) {
                __android_log_print(ANDROID_LOG_ERROR, "mesh", "Error loading diffuse texture %s",
                                    FullPath.c_str());
            } else {
                __android_log_print(ANDROID_LOG_ERROR, "mesh", "Loaded Diffuse texture");
            }
        }
    }
}


void Mesh::LoadSpecularTexture(const std::string &Dir, const aiMaterial *pMaterial, int index) {
    m_Materials[index].pSpecularExponent = NULL;

    if (pMaterial->GetTextureCount(aiTextureType_SHININESS) > 0) {
        aiString Path;

        if (pMaterial->GetTexture(aiTextureType_SHININESS, 0, &Path, NULL, NULL, NULL, NULL,
                                  NULL) == AI_SUCCESS) {
            std::string p(Path.data);

            if (p == "C:\\\\") {
                p = "";
            } else if (p.substr(0, 2) == ".\\") {
                p = p.substr(2, p.size() - 2);
            }
            // replaces incorrect path separator
            std::replace(p.begin(), p.end(), '\\', '/');

            std::string FullPath = Dir + "/" + p;
            __android_log_print(ANDROID_LOG_ERROR, "mesh", "specular path %s", FullPath.c_str());
            m_Materials[index].pSpecularExponent = new Texture(GL_TEXTURE_2D, FullPath.c_str(),
                                                               SPECULAR_TEXTURE_UNIT);

            if (!m_Materials[index].pSpecularExponent->load()) {
                __android_log_print(ANDROID_LOG_ERROR, "mesh", "Error loading specular texture %s",
                                    FullPath.c_str());
            } else {
                __android_log_print(ANDROID_LOG_ERROR, "mesh", "Loaded specular texture");
            }
        }
    }
}

void Mesh::LoadColors(const aiMaterial *pMaterial, int index) {
    aiColor3D AmbientColor(0.0f, 0.0f, 0.0f);
    Vector3f AllOnes(1.0f, 1.0f, 1.0f);

    int ShadingModel = 0;
    if (pMaterial->Get(AI_MATKEY_SHADING_MODEL, ShadingModel) == AI_SUCCESS) {
        printf("Shading model %d\n", ShadingModel);
    }

    if (pMaterial->Get(AI_MATKEY_COLOR_AMBIENT, AmbientColor) == AI_SUCCESS) {
        printf("Loaded ambient color [%f %f %f]\n", AmbientColor.r, AmbientColor.g, AmbientColor.b);
        m_Materials[index].AmbientColor.r = AmbientColor.r;
        m_Materials[index].AmbientColor.g = AmbientColor.g;
        m_Materials[index].AmbientColor.b = AmbientColor.b;
    } else {
        m_Materials[index].AmbientColor = AllOnes;
    }

    aiColor3D DiffuseColor(0.0f, 0.0f, 0.0f);

    if (pMaterial->Get(AI_MATKEY_COLOR_DIFFUSE, DiffuseColor) == AI_SUCCESS) {
        printf("Loaded diffuse color [%f %f %f]\n", DiffuseColor.r, DiffuseColor.g, DiffuseColor.b);
        m_Materials[index].DiffuseColor.r = DiffuseColor.r;
        m_Materials[index].DiffuseColor.g = DiffuseColor.g;
        m_Materials[index].DiffuseColor.b = DiffuseColor.b;
    }

    aiColor3D SpecularColor(0.0f, 0.0f, 0.0f);

    if (pMaterial->Get(AI_MATKEY_COLOR_SPECULAR, SpecularColor) == AI_SUCCESS) {
        printf("Loaded specular color [%f %f %f]\n", SpecularColor.r, SpecularColor.g,
               SpecularColor.b);
        m_Materials[index].SpecularColor.r = SpecularColor.r;
        m_Materials[index].SpecularColor.g = SpecularColor.g;
        m_Materials[index].SpecularColor.b = SpecularColor.b;
    }
}


void Mesh::PopulateBuffers() {
    GLCall(glBindBuffer(GL_ARRAY_BUFFER, m_Buffers[POS_VB]))
    GLCall(glBufferData(GL_ARRAY_BUFFER, m_Positions.size() * sizeof(m_Positions[0]),&m_Positions[0],GL_STATIC_DRAW))
    GLCall(glEnableVertexAttribArray(POSITION_LOCATION))
    GLCall(glVertexAttribPointer(POSITION_LOCATION, 3, GL_FLOAT, GL_FALSE, 0,nullptr))

    GLCall(glBindBuffer(GL_ARRAY_BUFFER, m_Buffers[TEXCOORD_VB]))
    GLCall(glBufferData(GL_ARRAY_BUFFER, m_TexCoords.size() * sizeof(m_TexCoords[0]),&m_TexCoords[0],GL_STATIC_DRAW))
    GLCall(glEnableVertexAttribArray(TEX_COORD_LOCATION))
    GLCall(glVertexAttribPointer(TEX_COORD_LOCATION, 2, GL_FLOAT, GL_FALSE, 0, nullptr))

    GLCall(glBindBuffer(GL_ARRAY_BUFFER, m_Buffers[NORMAL_VB]))
    GLCall(glBufferData(GL_ARRAY_BUFFER, m_Normals.size() * sizeof(m_Normals[0]), &m_Normals[0], GL_STATIC_DRAW))
    GLCall(glEnableVertexAttribArray(NORMAL_LOCATION))
    GLCall(glVertexAttribPointer(NORMAL_LOCATION, 3, GL_FLOAT, GL_FALSE, 0, nullptr))

    GLCall(glBindBuffer(GL_ARRAY_BUFFER, m_Buffers[BONE_VB]))
    GLCall(glBufferData(GL_ARRAY_BUFFER, m_Bones.size() * sizeof(m_Bones[0]), &m_Bones[0],GL_STATIC_DRAW))

    GLCall(glEnableVertexAttribArray(BONE_ID_LOCATION))
    GLCall(glVertexAttribIPointer(BONE_ID_LOCATION, MAX_NUM_BONES_PER_VERTEX, GL_INT,sizeof(VertexBoneData), nullptr))

    GLCall(glEnableVertexAttribArray(BONE_WEIGHT_LOCATION))
    GLCall(glVertexAttribPointer(BONE_WEIGHT_LOCATION, MAX_NUM_BONES_PER_VERTEX, GL_FLOAT, GL_FALSE,sizeof(VertexBoneData),(const GLvoid *) (MAX_NUM_BONES_PER_VERTEX * sizeof(int32_t))))

    GLCall(glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_Buffers[INDEX_BUFFER]))
    GLCall(glBufferData(GL_ELEMENT_ARRAY_BUFFER, m_Indices.size() * sizeof(m_Indices[0]), &m_Indices[0],GL_STATIC_DRAW))

    __android_log_print(ANDROID_LOG_ERROR, "Mesh", "Buffers populated");
}

void Mesh::Render(GLuint program) {
    GLCall(glBindVertexArray(m_VAO))

    for (auto &m_Meshe : m_Meshes) {

        unsigned int MaterialIndex = m_Meshe.MaterialIndex;

        if (m_Materials[MaterialIndex].pDiffuse) {
            Texture *texture = m_Materials[MaterialIndex].pDiffuse;
            GLCall(texture->bind())
            GLCall(GLuint diffuseLocation = glGetUniformLocation(program, "gSampler"))
            GLCall(glUniform1i(diffuseLocation, texture->getSlot()))
        }
        if (m_Materials[MaterialIndex].pSpecularExponent) {
            Texture *texture = m_Materials[MaterialIndex].pSpecularExponent;
            GLCall(GLuint specularLocation = glGetUniformLocation(program,
                           "gSamplerSpecularExponent"))
            GLCall(texture->bind())
            GLCall(glUniform1i(specularLocation, texture->getSlot()))
        }

        GLCall(glDrawElementsBaseVertex(GL_TRIANGLES, m_Meshe.NumIndices, GL_UNSIGNED_INT,
                                        (GLvoid *) (sizeof(unsigned int) * m_Meshe.BaseIndex),
                                        m_Meshe.BaseVertex))

    }

    // Necessary to unbind vao for the rest of the other program
    GLCall(glBindVertexArray(0))
}

const Material &Mesh::GetMaterial() {
    for (unsigned int i = 0; i < m_Materials.size(); i++) {
        if (m_Materials[i].AmbientColor != Vector3f(0.0f, 0.0f, 0.0f)) {
            return m_Materials[i];
        }
    }

    return m_Materials[0];
}


uint Mesh::FindPosition(float AnimationTimeTicks, const aiNodeAnim *pNodeAnim) {
    for (uint i = 0; i < pNodeAnim->mNumPositionKeys - 1; i++) {
        float t = (float) pNodeAnim->mPositionKeys[i + 1].mTime;
        if (AnimationTimeTicks < t) {
            return i;
        }
    }

    return 0;
}


void Mesh::CalcInterpolatedPosition(aiVector3D &Out, float AnimationTimeTicks,
                                    const aiNodeAnim *pNodeAnim) {
    // we need at least two values to interpolate...
    if (pNodeAnim->mNumPositionKeys == 1) {
        Out = pNodeAnim->mPositionKeys[0].mValue;
        return;
    }

    uint PositionIndex = FindPosition(AnimationTimeTicks, pNodeAnim);
    uint NextPositionIndex = PositionIndex + 1;
    assert(NextPositionIndex < pNodeAnim->mNumPositionKeys);
    float t1 = (float) pNodeAnim->mPositionKeys[PositionIndex].mTime;
    float t2 = (float) pNodeAnim->mPositionKeys[NextPositionIndex].mTime;
    float DeltaTime = t2 - t1;
    float Factor = (AnimationTimeTicks - t1) / DeltaTime;
    assert(Factor >= 0.0f && Factor <= 1.0f);
    const aiVector3D &Start = pNodeAnim->mPositionKeys[PositionIndex].mValue;
    const aiVector3D &End = pNodeAnim->mPositionKeys[NextPositionIndex].mValue;
    aiVector3D Delta = End - Start;
    Out = Start + Factor * Delta;
}


uint Mesh::FindRotation(float AnimationTimeTicks, const aiNodeAnim *pNodeAnim) {
    assert(pNodeAnim->mNumRotationKeys > 0);

    for (uint i = 0; i < pNodeAnim->mNumRotationKeys - 1; i++) {
        float t = (float) pNodeAnim->mRotationKeys[i + 1].mTime;
        if (AnimationTimeTicks < t) {
            return i;
        }
    }

    return 0;
}


void Mesh::CalcInterpolatedRotation(aiQuaternion &Out, float AnimationTimeTicks,
                                    const aiNodeAnim *pNodeAnim) {
    // we need at least two values to interpolate...
    if (pNodeAnim->mNumRotationKeys == 1) {
        Out = pNodeAnim->mRotationKeys[0].mValue;
        return;
    }

    uint RotationIndex = FindRotation(AnimationTimeTicks, pNodeAnim);
    uint NextRotationIndex = RotationIndex + 1;
    assert(NextRotationIndex < pNodeAnim->mNumRotationKeys);
    float t1 = (float) pNodeAnim->mRotationKeys[RotationIndex].mTime;
    float t2 = (float) pNodeAnim->mRotationKeys[NextRotationIndex].mTime;
    float DeltaTime = t2 - t1;
    float Factor = (AnimationTimeTicks - t1) / DeltaTime;
    assert(Factor >= 0.0f && Factor <= 1.0f);
    const aiQuaternion &StartRotationQ = pNodeAnim->mRotationKeys[RotationIndex].mValue;
    const aiQuaternion &EndRotationQ = pNodeAnim->mRotationKeys[NextRotationIndex].mValue;
    aiQuaternion::Interpolate(Out, StartRotationQ, EndRotationQ, Factor);
    Out = StartRotationQ;
    Out.Normalize();
}


uint Mesh::FindScaling(float AnimationTimeTicks, const aiNodeAnim *pNodeAnim) {
    assert(pNodeAnim->mNumScalingKeys > 0);

    for (uint i = 0; i < pNodeAnim->mNumScalingKeys - 1; i++) {
        float t = (float) pNodeAnim->mScalingKeys[i + 1].mTime;
        if (AnimationTimeTicks < t) {
            return i;
        }
    }

    return 0;
}


void Mesh::CalcInterpolatedScaling(aiVector3D &Out, float AnimationTimeTicks,
                                   const aiNodeAnim *pNodeAnim) {
    // we need at least two values to interpolate...
    if (pNodeAnim->mNumScalingKeys == 1) {
        Out = pNodeAnim->mScalingKeys[0].mValue;
        return;
    }

    uint ScalingIndex = FindScaling(AnimationTimeTicks, pNodeAnim);
    uint NextScalingIndex = ScalingIndex + 1;
    assert(NextScalingIndex < pNodeAnim->mNumScalingKeys);
    float t1 = (float) pNodeAnim->mScalingKeys[ScalingIndex].mTime;
    float t2 = (float) pNodeAnim->mScalingKeys[NextScalingIndex].mTime;
    float DeltaTime = t2 - t1;
    float Factor = (AnimationTimeTicks - (float) t1) / DeltaTime;
    assert(Factor >= 0.0f && Factor <= 1.0f);
    const aiVector3D &Start = pNodeAnim->mScalingKeys[ScalingIndex].mValue;
    const aiVector3D &End = pNodeAnim->mScalingKeys[NextScalingIndex].mValue;
    aiVector3D Delta = End - Start;
    Out = Start + Factor * Delta;
}


void Mesh::ReadNodeHierarchy(float AnimationTimeTicks, const aiNode *pNode,
                             const Matrix4f &ParentTransform) {
    std::string NodeName(pNode->mName.data);

    const aiAnimation *pAnimation = pScene->mAnimations[0];

    Matrix4f NodeTransformation(pNode->mTransformation);

    const aiNodeAnim *pNodeAnim = FindNodeAnim(pAnimation, NodeName);

    if (pNodeAnim) {
        // Interpolate scaling and generate scaling transformation matrix
        aiVector3D Scaling;
        CalcInterpolatedScaling(Scaling, AnimationTimeTicks, pNodeAnim);
        Matrix4f ScalingM;
        ScalingM.InitScaleTransform(Scaling.x, Scaling.y, Scaling.z);

        // Interpolate rotation and generate rotation transformation matrix
        aiQuaternion RotationQ;
        CalcInterpolatedRotation(RotationQ, AnimationTimeTicks, pNodeAnim);
        Matrix4f RotationM = Matrix4f(RotationQ.GetMatrix());

        // Interpolate translation and generate translation transformation matrix
        aiVector3D Translation;
        CalcInterpolatedPosition(Translation, AnimationTimeTicks, pNodeAnim);
        Matrix4f TranslationM;
        TranslationM.InitTranslationTransform(Translation.x, Translation.y, Translation.z);

        // Combine the above transformations
        NodeTransformation = TranslationM * RotationM * ScalingM;
    }

    Matrix4f GlobalTransformation = ParentTransform * NodeTransformation;

    if (m_BoneNameToIndexMap.find(NodeName) != m_BoneNameToIndexMap.end()) {
        uint BoneIndex = m_BoneNameToIndexMap[NodeName];
        m_BoneInfo[BoneIndex].FinalTransformation =
                m_GlobalInverseTransform * GlobalTransformation *
                m_BoneInfo[BoneIndex].OffsetMatrix;
    }

    for (uint i = 0; i < pNode->mNumChildren; i++) {
        ReadNodeHierarchy(AnimationTimeTicks, pNode->mChildren[i], GlobalTransformation);
    }
}

void Mesh::GetBoneTransforms(float TimeInSeconds, std::vector<Matrix4f> &Transforms) {
    Matrix4f Identity;
    Identity.InitIdentity();
    float TicksPerSecond = (float) (pScene->mAnimations[0]->mTicksPerSecond != 0
                                    ? pScene->mAnimations[0]->mTicksPerSecond : 25.0f);
    float TimeInTicks = TimeInSeconds * TicksPerSecond;
    float AnimationTimeTicks = fmod(TimeInTicks, (float) pScene->mAnimations[0]->mDuration);
    ReadNodeHierarchy(AnimationTimeTicks, pScene->mRootNode, Identity);
    Transforms.resize(m_BoneInfo.size());
    for (uint i = 0; i < m_BoneInfo.size(); i++) {
        Transforms[i] = m_BoneInfo[i].FinalTransformation;
    }
}


const aiNodeAnim *Mesh::FindNodeAnim(const aiAnimation *pAnimation, const std::string &NodeName) {
    for (uint i = 0; i < pAnimation->mNumChannels; i++) {
        const aiNodeAnim *pNodeAnim = pAnimation->mChannels[i];

        if (std::string(pNodeAnim->mNodeName.data) == NodeName) {
            return pNodeAnim;
        }
    }

    return NULL;
}

float Mesh::getAnimationSecond() const {
    return (currentTimeInMilliseconds() - startTimeInMillis) / 1000.0f;
}

//void Mesh::Render() {
//    glBindVertexArray(m_VAO);
//
//    glEnableVertexAttribArray(0);
//    glEnableVertexAttribArray(1);
//    glEnableVertexAttribArray(2);
//
//    for (unsigned int i = 0; i < m_Entries.size(); i++) {
//        glBindBuffer(GL_ARRAY_BUFFER, m_Entries[i].VB);
//        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), nullptr);
//        glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (const GLvoid *) 12);
//        glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (const GLvoid *) 20);
//
//        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_Entries[i].IB);
//
//        const unsigned int MaterialIndex = m_Entries[i].MaterialIndex;
//
//        if (MaterialIndex < m_Textures.size() && m_Textures[MaterialIndex]) {
//            m_Textures[MaterialIndex]->bind();
//        }
//
//        glDrawElements(GL_TRIANGLES, m_Entries[i].NumIndices, GL_UNSIGNED_INT, nullptr);
//    }
//
//    glDisableVertexAttribArray(0);
//    glDisableVertexAttribArray(1);
//    glDisableVertexAttribArray(2);
//
//    glBindVertexArray(0);
//}