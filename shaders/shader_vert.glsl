#version 410

uniform mat4 mvpMatrix;
uniform mat4 modelMatrix;
// Normals should be transformed differently than positions:
// https://paroj.github.io/gltut/Illumination/Tut09%20Normal%20Transformation.html
uniform mat3 normalModelMatrix;

layout(location = 0) in vec3 position;
layout(location = 1) in vec3 normal;
layout(location = 2) in vec2 texCoord;
layout(location = 3) in vec3 tangent;
layout(location = 4) in vec3 bitangent;

out vec3 fragPosition;
out vec3 fragNormal;
out vec2 fragTexCoord;
out mat3 TBN;

void main()
{
    gl_Position = mvpMatrix * vec4(position, 1);
    
    fragPosition    = vec3(modelMatrix* vec4(position,1));
    fragNormal      = normalModelMatrix * normal;
    fragTexCoord    = texCoord;
    
//    vec3 T = normalize(vec3(modelMatrix * vec4(tangent, 0.0)));
    vec3 T = normalize(normalModelMatrix * tangent);
    vec3 B = normalize(normalModelMatrix * bitangent);
    
    TBN             = mat3(T, B, normal);
}
