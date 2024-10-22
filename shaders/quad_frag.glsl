#version 410 core

out vec4 fragColor;  // Output color

in vec2 TexCoord;  // Interpolated texture coordinates from the vertex shader

uniform sampler2D texture1;  // The texture sampler
uniform sampler2D overlay;

void main()
{
    // Sample the color from the texture at the given coordinates

    vec4 overlayColor = texture(overlay, TexCoord);
    if(overlayColor.r == 1.f) fragColor = texture(texture1, TexCoord); 
    else fragColor = overlayColor;
}
