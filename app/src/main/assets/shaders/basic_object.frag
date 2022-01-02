#version 320 es
precision mediump float;

in vec2 TexCoord0;
uniform sampler2D texture;
out vec4 FragColor;
out vec3 Normal0;

void main()
{
    // vec4 normals = vec4(normalize(Normal0), 1.0f);
    FragColor = texture(texture, TexCoord0);
}