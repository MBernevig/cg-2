#version 410

layout(std140) uniform Material // Must match the GPUMaterial defined in src/mesh.h
{
    vec3 kd;
	vec3 ks;
	float shininess;
	float transparency;
};

struct Light {
    vec4 position;
    vec4 color;
};

layout(std140) uniform lightBuffer {
    int light_count;
    Light lights[32];
};

uniform sampler2D colorMap;
uniform bool hasTexCoords;
uniform bool useMaterial;

uniform vec3 cameraPosition;

in vec3 fragPosition;
in vec3 fragNormal;
in vec2 fragTexCoord;

layout(location = 0) out vec4 fragColor;

void main()
{
    vec3 normal = normalize(fragNormal);

    vec3 fullColor;

    if (hasTexCoords)       { fullColor = vec3(texture(colorMap, fragTexCoord).rgb);}
    else if (useMaterial)   { fullColor = vec3(kd);}
    else                    { fragColor = vec4(normal, 1); return;} // Output color value, change from (1, 0, 0) to something else

    fragColor = vec4(0.f);

    for(int i = 0; i<light_count; i++){
        vec3 lightDir = normalize(lights[i].position.rgb - fragPosition);
        float diff = max(dot(normal, lightDir), 0.0);
        vec3 diffuse = diff * lights[i].color.rgb * fullColor;

        fragColor += vec4(diffuse, 1.f);
    }

    fragColor = vec4(fullColor, 1.f);
}
