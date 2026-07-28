// scene.frag — placeholder shading: Lambert + ambient. Procedural checker
// pattern on the ground plane. Project 10 (Advanced Shaders 2) replaces the
// per-mesh logic with environment / parallax / bump mapping.
#version 410 core
in vec3 vNormal;
in vec3 vWorldPos;
in vec2 vUV;
out vec4 FragColor;
uniform vec3 uLightDir;
uniform vec3 uColor;
uniform int  uIsCheckerboard;

void main() {
    vec3 base = uColor;
    if(uIsCheckerboard == 1) {
        // 1-meter check size — built from UV grid
        bool tile = (mod(floor(vUV.x) + floor(vUV.y), 2.0)) < 0.5;
        base = tile ? vec3(0.86, 0.84, 0.78) : vec3(0.18, 0.20, 0.25);
    }
    vec3 N = normalize(vNormal);
    vec3 L = normalize(uLightDir);
    float NdotL = max(dot(N, L), 0.0);
    vec3 color = base * (0.18 + 0.82 * NdotL);
    FragColor = vec4(color, 1.0);
}
