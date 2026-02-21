#version 300 es

precision mediump float;

layout (location = 0) in vec3 aPos;
layout (location = 1) in vec2 aTexCoords;
layout (location = 2) in vec4 aColor;

out vec2 TexCoord;
out vec4 oColor;

uniform mat4 mvp;

void main() {
    gl_Position = mvp * vec4(aPos.xyz, 1.0);
    oColor = aColor;
    TexCoord = aTexCoords;
}