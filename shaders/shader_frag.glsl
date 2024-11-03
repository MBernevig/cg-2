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
    mat4 lightMVP;
};

layout(std140) uniform lightBuffer {
    int light_count;
    Light lights[32];
};

uniform sampler2D shadowMap;

uniform sampler2D colorMap;
uniform bool hasTexCoords;
uniform bool useMaterial;

uniform vec3 cameraPosition;

in vec3 fragPosition;
in vec3 fragNormal;
in vec2 fragTexCoord;

layout(location = 0) out vec4 fragColor;

vec3 get_frag_light_coord(int lightIndex)
{
    vec4 fragLightCoord = lights[lightIndex].lightMVP * vec4(fragPosition, 1.0);

    // Divide by w because fragLightCoord are homogeneous coordinates
    fragLightCoord.xyz /= fragLightCoord.w;

    // The resulting value is in NDC space (-1 to +1),
    //  we transform them to texture space (0 to 1).
    fragLightCoord.xyz = fragLightCoord.xyz * 0.5 + 0.5;

    return fragLightCoord.xyz;
}

bool shadow_test(int lightIndex)
{
    vec3 fragLightCoord = get_frag_light_coord(lightIndex);

    // Depth of the fragment with respect to the light
    float fragLightDepth = fragLightCoord.z;

    // Shadow map coordinate corresponding to this fragment
    vec2 shadowMapCoord = fragLightCoord.xy;

    // Shadow map value from the corresponding shadow map position
    float shadowMapDepth = texture(shadowMap, shadowMapCoord).x;

    return (shadowMapDepth + 0.001 <= fragLightDepth);
}

void main()
{
    vec3 normal = normalize(fragNormal);

    vec3 fullColor;

    if (hasTexCoords)       { fullColor = vec3(texture(colorMap, fragTexCoord).rgb);}
    else if (useMaterial)   { fullColor = vec3(kd);}
    else                    { fragColor = vec4(normal, 1); return;} // Output color value, change from (1, 0, 0) to something else

    fragColor = vec4(0.f);

    for(int i = 0; i < light_count; i++){
        vec3 lightDir = normalize(lights[i].position.rgb - fragPosition);
        float diff = max(dot(normal, lightDir), 0.0);
        vec3 diffuse = diff * lights[i].color.rgb * fullColor;
        
        if (shadow_test(i)) {
            fragColor -= vec4(1.0, 1.0, 1.0, 1.0) / light_count;
        } else {
            // lambert
            fragColor += lights[i].color * vec4(kd * dot(fragNormal, normalize(lights[i].position.xyz - fragPosition)), 1.0);
            // blinn-phong
            vec3 L = normalize(lights[i].position.xyz - fragPosition);
            vec3 V = normalize(cameraPosition - fragPosition);
            vec3 H = normalize(V + L);
            float LH_dot = dot(fragNormal, H);
            float NV_dot = dot(fragNormal, V);
            fragColor += LH_dot < 0 || NV_dot < 0 ? vec4(0.0) : lights[i].color * vec4(ks * pow(LH_dot, shininess), 1.0);
        }
    }
}
