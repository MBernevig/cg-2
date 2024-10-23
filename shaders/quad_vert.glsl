#version 410 core

layout(location = 0) in vec3 aPos;  // Vertex position
layout(location = 1) in vec2 aTexCoord;  // Texture coordinates

out vec2 TexCoord;  // Pass the texture coordinates to the fragment shader

uniform mat4 mvpMatrix;

void main()
{
    // Transform the vertex position using model, view, and projection matrices
    gl_Position = mvpMatrix * vec4(aPos, 1);
    
    // Pass the texture coordinates to the fragment shader
    TexCoord = aTexCoord;
}
