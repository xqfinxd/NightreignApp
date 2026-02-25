#version 300 es

precision highp float;

layout(location = 0) in vec3 aPosition;
layout(location = 1) in vec2 aTexCoord;
layout(location = 2) in vec4 aColor;

out vec2 TexCoord;
out vec4 FragColor;

uniform mat4 mvp;

void main()
{
    gl_Position = mvp * vec4(aPosition, 1.0);
    TexCoord = aTexCoord;
    FragColor = aColor;
}
