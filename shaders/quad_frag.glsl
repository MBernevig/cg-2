#version 410 core

out vec4 fragColor;  // Output color

in vec2 TexCoord;  // Interpolated texture coordinates from the vertex shader

uniform sampler2D texture1;  // The texture sampler
uniform sampler2D overlay;

void main()
{
    // Sample the color from the texture at the given coordinates
    

    vec4 overlayColor = texture(overlay, (-1.f) * TexCoord);
    if(overlayColor.r == 1.f) fragColor = texture(texture1, TexCoord); 
    else fragColor = overlayColor;

    if(TexCoord == vec2(0.5f,0.5f)) fragColor = vec4(1.f);
    else fragColor = texture(texture1, TexCoord); 
}
