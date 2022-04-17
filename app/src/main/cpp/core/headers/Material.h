//
// Created by Roshan on 24/03/22.
//

#ifndef OPENGL_MATERIAL_H
#define OPENGL_MATERIAL_H

class Material {

public:
    Vector3f AmbientColor = Vector3f(0.0f, 0.0f, 0.0f);
    Vector3f DiffuseColor = Vector3f(0.0f, 0.0f, 0.0f);
    Vector3f SpecularColor = Vector3f(0.0f, 0.0f, 0.0f);

    // TODO: need to deallocate these
    Texture *pDiffuse = NULL; // base color of the material
    Texture *pSpecularExponent = NULL;
};

#endif //OPENGL_MATERIAL_H
