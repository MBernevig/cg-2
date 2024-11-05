#version 410


layout (location = 0) in vec3 aPos;

out vec3 texCoords;

uniform mat4 projectionSkybox;
uniform mat4 viewSkybox;

void main()
{
    texCoords = vec3(aPos.x, aPos.y, -aPos.z);
    vec4 pos = projectionSkybox * viewSkybox * vec4(aPos, 1.0);
    gl_Position = pos.xyww;
}  