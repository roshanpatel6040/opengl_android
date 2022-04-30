#version 320 es

layout(location = 0) vec4 vPosition;

uniform mat4 uMVPMatrix;

void main()
{
    gl_Position = uMVPMatrix * vPosition;
}