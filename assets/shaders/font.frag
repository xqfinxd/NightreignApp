#version 330 core
in vec2 TexCoord;
out vec4 FragColor;

uniform sampler2D fontTexture;
uniform vec4 foregroundColor;
uniform vec4 backgroundColor;

void main()
{
    // Sample the alpha value from the font texture (red channel for grayscale font atlas)
    float alpha = texture(fontTexture, TexCoord).a;
    
    // Mix between background and foreground color based on alpha
    FragColor = mix(backgroundColor, foregroundColor, alpha);
}
