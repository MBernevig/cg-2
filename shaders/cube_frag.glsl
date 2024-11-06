#version 410
out vec4 fragColor;

in vec3 fragNormal;
in vec3 fragPosition;

uniform vec3 cameraPosition;
uniform samplerCube skybox;
uniform int reflectMode;
uniform float refractionIndex;

void main()
{             
    vec3 I = normalize(fragPosition - cameraPosition);
    if(reflectMode == 1) {
        vec3 R = reflect(I, normalize(fragNormal));
        R.z = -R.z;
        fragColor = vec4(texture(skybox, R).rgb, 1.0);
    } else {
        float ratio = 1.00 / refractionIndex;
        vec3 R = refract(I, normalize(fragNormal), ratio);
        R.z = -R.z;
        fragColor = vec4(texture(skybox, R).rgb, 1.0);
    }
}