#version 410 core

layout(location = 0) in vec3 aPos;  // Vertex position
layout(location = 1) in vec2 aTexCoord;  // Texture coordinates

out vec2 TexCoord;  // Pass the texture coordinates to the fragment shader

uniform mat4 mvpMatrix;

void main()
{
    // Transform the vertex position using model, view, and projection matrices
    vec4 transformedPos = mvpMatrix * vec4(aPos, 1);
    
    // Set the depth to always be at 0
    gl_Position = vec4(transformedPos.xy, 0.0, transformedPos.w);
    
    // Pass the texture coordinates to the fragment shader
    TexCoord = aTexCoord;
}
