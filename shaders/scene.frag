// scene.frag — basic per-object shading for Project 9. Each object gets a
// distinct, lightweight technique that Project 10 (Advanced Shaders 2) will
// replace with the full version of the same idea:
//   mode 0 (ground)    procedural checkerboard, Lambert + ambient
//   mode 1 (sphere)    Lambert + a Fresnel rim highlight — a cheap stand-in
//                       for reflectivity, ahead of real environment mapping
//   mode 2 (cylinder)  procedural bump mapping (analytic-derivative ridge
//                       pattern), ahead of a refined bump/normal map
//   mode 3 (cube)      a procedural panel/seam pattern giving the surface
//                       visible structure, ahead of true parallax mapping
#version 410 core
in vec3 vNormal;
in vec3 vWorldPos;
in vec2 vUV;
out vec4 FragColor;

uniform vec3 uLightDir;
uniform vec3 uEyePos;
uniform vec3 uColor;
uniform int  uShaderMode; // 0=checkerboard, 1=sphere-fresnel, 2=cylinder-bump, 3=cube-panel

// ---- mode 2: procedural bump map (same technique verified in Lab Q3) ----
const float RIDGE_FREQ = 40.0;
const float RIDGE_AMP  = 0.05;
float bumpHeightDv(vec2 uv) {
    // Analytic d/dv of sin(v*RIDGE_FREQ)*RIDGE_AMP — using the closed-form
    // derivative (not a screen-space dFdx/dFdy of the height itself) keeps
    // the ridge strength constant regardless of camera distance.
    return cos(uv.y * RIDGE_FREQ) * RIDGE_FREQ * RIDGE_AMP;
}

void main() {
    vec3 N = normalize(vNormal);
    vec3 L = normalize(uLightDir);
    vec3 V = normalize(uEyePos - vWorldPos);
    vec3 base = uColor;

    if (uShaderMode == 0) {
        // 1-meter check size — built from UV grid
        bool tile = (mod(floor(vUV.x) + floor(vUV.y), 2.0)) < 0.5;
        base = tile ? vec3(0.86, 0.84, 0.78) : vec3(0.18, 0.20, 0.25);
        float diff = max(dot(N, L), 0.0);
        FragColor = vec4(base * (0.18 + 0.82 * diff), 1.0);
        return;
    }

    if (uShaderMode == 1) {
        // Fresnel rim: grazing angles (N nearly perpendicular to V) brighten
        // toward white, hinting at a reflective surface without an actual
        // environment/cube map — Project 10 replaces this with real
        // environment mapping.
        float diff = max(dot(N, L), 0.0);
        float fres = pow(1.0 - max(dot(N, V), 0.0), 3.0);
        vec3 lit = base * (0.18 + 0.82 * diff);
        vec3 color = mix(lit, vec3(1.0), fres * 0.55);
        FragColor = vec4(color, 1.0);
        return;
    }

    if (uShaderMode == 2) {
        // Procedural bump mapping: perturb the shading normal along the
        // bitangent by the ridge height field's analytic slope.
        vec3 T = normalize(dFdx(vWorldPos));
        vec3 B = normalize(cross(N, T));
        T = normalize(cross(B, N));
        const float STRENGTH = 6.0;
        vec3 bumpedN = normalize(N - B * bumpHeightDv(vUV) * STRENGTH);
        float diff = max(dot(bumpedN, L), 0.0);
        FragColor = vec4(base * (0.18 + 0.82 * diff), 1.0);
        return;
    }

    // uShaderMode == 3: procedural panel/seam pattern — cheap surface detail
    // cue ahead of true parallax (height-map UV displacement) mapping.
    vec2 g = abs(fract(vUV * 4.0 - 0.5) - 0.5);
    float seam = smoothstep(0.0, 0.05, min(g.x, g.y));
    vec3 panel = mix(base * 0.55, base, seam);
    float diff = max(dot(N, L), 0.0);
    FragColor = vec4(panel * (0.18 + 0.82 * diff), 1.0);
}
