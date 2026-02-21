#version 300 es

precision mediump float;

in vec2 TexCoord;
in vec4 oColor;
out vec4 FragColor;

uniform sampler2D mapTexture;

void main() {
    FragColor = oColor * texture(mapTexture, TexCoord);
}