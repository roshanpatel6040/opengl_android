//
// Created by Roshan on 21/12/21.
//

#ifndef OPENGL_UTILS_H
#define OPENGL_UTILS_H

#include <cmath>
#include <math.h>
#include <stdio.h>

#include <assimp/vector3.h>
#include <assimp/matrix3x3.h>
#include <assimp/matrix4x4.h>
#include <glm.hpp>
#include <cstring>
#include <iostream>
#include <GLES3/gl3.h>
#include <GLES2/gl2.h>
#include <GLES2/gl2platform.h>

#define ARRAY_SIZE_IN_ELEMENTS(a) (sizeof(a)/sizeof(a[0]))
#define ZERO_MEM(a) memset(a, 0, sizeof(a))

#define DIFFUSE_TEXTURE_UNIT 5
#define SPECULAR_TEXTURE_UNIT 6

#define COLOR_TEXTURE_UNIT              GL_TEXTURE0
#define COLOR_TEXTURE_UNIT_INDEX        0
#define SHADOW_TEXTURE_UNIT             GL_TEXTURE1
#define SHADOW_TEXTURE_UNIT_INDEX       1
#define NORMAL_TEXTURE_UNIT             GL_TEXTURE2
#define NORMAL_TEXTURE_UNIT_INDEX       2
#define RANDOM_TEXTURE_UNIT             GL_TEXTURE3
#define RANDOM_TEXTURE_UNIT_INDEX       3
#define DISPLACEMENT_TEXTURE_UNIT       GL_TEXTURE4
#define DISPLACEMENT_TEXTURE_UNIT_INDEX 4
#define MOTION_TEXTURE_UNIT             GL_TEXTURE5
#define MOTION_TEXTURE_UNIT_INDEX       5
#define SPECULAR_EXPONENT_UNIT             GL_TEXTURE6
#define SPECULAR_EXPONENT_UNIT_INDEX       6
#define CASCACDE_SHADOW_TEXTURE_UNIT0       SHADOW_TEXTURE_UNIT
#define CASCACDE_SHADOW_TEXTURE_UNIT0_INDEX SHADOW_TEXTURE_UNIT_INDEX
#define CASCACDE_SHADOW_TEXTURE_UNIT1       GL_TEXTURE6
#define CASCACDE_SHADOW_TEXTURE_UNIT1_INDEX 6
#define CASCACDE_SHADOW_TEXTURE_UNIT2       GL_TEXTURE7
#define CASCACDE_SHADOW_TEXTURE_UNIT2_INDEX 7

struct BaseLight {
    glm::vec3 Color;
    float AmbientIntensity;
    float DiffuseIntensity;
};

struct DirectionalLight {
    BaseLight Base;
    glm::vec3 Direction;
};

struct Attenuation {
    float Constant;
    float Linear;
    float Exp;
};

struct PointLight {
    BaseLight Base;
    glm::vec3 Position;
    Attenuation Atten;
};

struct SpotLight {
    PointLight Base;
    glm::vec3 Direction;
    float Cutoff;
};

#endif

