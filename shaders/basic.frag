#version 330 core

in vec3 vWorldPos;
in vec3 vNormal;

uniform vec3 uLightDir;   // direction the light travels, world space
uniform vec3 uCameraPos;
uniform vec3 uColor;

out vec4 fragColor;

// Blinn-Phong shading: a simple baseline to compare new techniques against.
void main() {
    vec3 n = normalize(vNormal);
    vec3 l = normalize(-uLightDir);
    vec3 v = normalize(uCameraPos - vWorldPos);
    vec3 h = normalize(l + v);

    float ambient = 0.15;
    float diffuse = max(dot(n, l), 0.0);
    float specular = pow(max(dot(n, h), 0.0), 64.0) * (diffuse > 0.0 ? 1.0 : 0.0);

    vec3 color = uColor * (ambient + diffuse) + vec3(0.4) * specular;
    fragColor = vec4(pow(color, vec3(1.0 / 2.2)), 1.0);  // gamma correction
}
