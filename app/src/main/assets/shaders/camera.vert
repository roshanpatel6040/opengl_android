#version 320 es
in vec3 position;
in vec2 texChords;
uniform mat4 texMatrix;
uniform mat4 u_MVP;
out vec2 v_Chord;
void main()
{
    v_Chord = (texMatrix * vec4(texChords.x, texChords.y, 0.0, 1.0)).xy;
    gl_Position = u_MVP * vec4(position, 1.0);
}