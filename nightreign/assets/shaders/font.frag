#version 300 es

precision highp float;

in vec2 TexCoord;
in vec4 FragColor;
out vec4 Out_Color;

uniform sampler2D fontTexture;
uniform vec4 backgroundColor;

void main()
{
    vec4 pixelColor = max(backgroundColor, texture(fontTexture, TexCoord));
    Out_Color =  FragColor * pixelColor;
}
