// taken from assg 1_1
#version 410
layout(location = 0) in vec4 pos;
uniform mat4 mvp;

void main()
{
    gl_PointSize = 40.0;
    gl_Position = mvp * vec4(pos.xyz, 1.0);
}

