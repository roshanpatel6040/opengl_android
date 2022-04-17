//
// Created by Roshan on 24/03/22.
//

#ifndef OPENGL_WORLDTRANSFORM_H
#define OPENGL_WORLDTRANSFORM_H

#include "MathHelper.h"

class WorldTrans {
public:
    WorldTrans() {}

    void SetScale(float scale);

    void SetRotation(float x, float y, float z);

    void SetRotation(const Vector3f &);

    void SetPosition(float x, float y, float z);

    void SetPosition(const Vector3f &WorldPos);

    void Rotate(float x, float y, float z);

    Matrix4f GetMatrix() const;

    Vector3f WorldPosToLocalPos(const Vector3f &WorldPos) const;

    Vector3f WorldDirToLocalDir(const Vector3f &WorldDir) const;

    Matrix4f GetReversedTranslationMatrix() const;

    Matrix4f GetReversedRotationMatrix() const;

    float GetScale() const { return m_scale; }

    Vector3f GetPos() const { return m_pos; }

    Vector3f GetRotation() const { return m_rotation; }

private:
    float m_scale = 1.0f;
    Vector3f m_rotation = Vector3f(0.0f, 0.0f, 0.0f);
    Vector3f m_pos = Vector3f(0.0f, 0.0f, 0.0f);
};


#endif //OPENGL_WORLDTRANSFORM_H
