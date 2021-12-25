#version 320 es
#extension GL_OES_EGL_image_external_essl3:require
precision highp float;
in lowp vec2 v_Chord;
uniform samplerExternalOES tex;
out vec4 fragColor;
void main()
{
    fragColor = texture(tex, v_Chord);
}