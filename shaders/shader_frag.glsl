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

uniform mat4 meshModelMatrix;

layout(std140) uniform lightBuffer {
    int light_count;
    Light lights[32];
};

uniform sampler2D shadowMap[10];

uniform samplerCube skybox;

uniform sampler2D colorMap;
uniform sampler2D normalMap;
uniform bool hasTexCoords;
uniform bool useMaterial;
uniform bool useNormalMap;

uniform vec3 cameraPosition;

in vec3 fragPosition;
in vec3 fragNormal;
in vec2 fragTexCoord;
in mat3 TBN;

uniform int shadowMode;
uniform int sampleCount;

layout(location = 0) out vec4 fragColor;




vec3 get_frag_light_coord(int lightIndex)
{
    vec4 fragLightCoord = lights[lightIndex].lightMVP * meshModelMatrix * vec4(fragPosition,1.f);

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
    
    // Outside the texture, so return false to prevent what's outside the lights fov from being in shadow
    if (fragLightDepth > 1.0)
        return false;

    // Shadow map coordinate corresponding to this fragment
    vec2 shadowMapCoord = fragLightCoord.xy;

    // Shadow map value from the corresponding shadow map position
    float shadowMapDepth = texture(shadowMap[lightIndex], shadowMapCoord).x;

    return (shadowMapDepth + 0.0001 <= fragLightDepth);
}

float computeSoftShadow(int lightIndex)
{
    vec3 fragLightCoord = get_frag_light_coord(lightIndex);

    if (fragLightCoord.z > 1.0) {
        return 1.0; // Outside light's range, no shadow
    }

    float shadowFactor = 0.0;
    float radius = 1.0 / 1024.0; 
    int totalSamples = 0;
    
    vec3 norm = normalize(fragNormal);
    vec3 lightDir = normalize(lights[lightIndex].position.xyz - fragPosition);
    float diff = max(dot(norm, lightDir), 0.0);

    float bias = max(0.0001f * (1.0 - dot(norm, lightDir)), 0.0001f);

    for (int x = -sampleCount; x <= sampleCount; x++) {
        for (int y = -sampleCount; y <= sampleCount; y++) {
            vec2 offset = vec2(x, y) * radius;
            float shadowMapDepth = texture(shadowMap[lightIndex], fragLightCoord.xy + offset).r;
            shadowFactor += (fragLightCoord.z > shadowMapDepth + bias) ? 0.0 : 1.0;
            totalSamples++;
        }
    }
    return shadowFactor / totalSamples; // Normalize the shadow factor
}

void main()
{
    vec3 normal = normalize(fragNormal);

    vec3 fullColor;

    if (hasTexCoords)       { fullColor = vec3(texture(colorMap, fragTexCoord).rgb);}
    else if (useMaterial)   { fullColor = vec3(kd);}
    else                    { fragColor = vec4(normal, 1); return;} // Output color value, change from (1, 0, 0) to something else
    
    if (useNormalMap)       {
        vec3 texNormal = texture(normalMap, fragTexCoord).rgb * 2.0 - 1.0;
        texNormal.y = -texNormal.y; // 💀
        normal = normalize(TBN * texNormal); 
    }

    fragColor = vec4(vec3(0.1f) * fullColor, 1.f);
    float shadowFactor = 1.f;

    for(int i = 0; i < light_count; i++){
        if(shadowMode == 0) {
            shadowFactor = 1;
        } else if(shadow_test(i)) {
            if(shadowMode == 1) continue;
            else shadowFactor = computeSoftShadow(i);
        }
        // lambert
        fragColor += shadowFactor * lights[i].color * vec4(fullColor * dot(normal, normalize(lights[i].position.xyz - fragPosition)), 1.0);
        // blinn-phong
        vec3 L = normalize(lights[i].position.xyz - fragPosition);
        vec3 V = normalize(cameraPosition - fragPosition);
        vec3 H = normalize(V + L);
        float LH_dot = dot(normal, H);
        float NV_dot = dot(normal, V);

        vec3 I = normalize(fragPosition - cameraPosition);
        vec3 R = reflect(I, normalize(normal));
        R.z = -R.z;
        
        fragColor += LH_dot < 0 || NV_dot < 0 ? vec4(0.0) : shadowFactor * lights[i].color * vec4(ks * pow(LH_dot, shininess), 1.0) * vec4(texture(skybox, R).rgb, 1.0);
        
    }
    // fragColor /= light_count; // blending
    fragColor.a = 1.0; // Set alpha to 1.0
}
