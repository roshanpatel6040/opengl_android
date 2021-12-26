#version 320 es
precision mediump float;

in vec2 TexCoord0;
uniform sampler2D texture;
out vec4 FragColor;

void main()
{
    FragColor = texture(texture, TexCoord0);
}