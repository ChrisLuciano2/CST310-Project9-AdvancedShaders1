#version 410 core
layout(location = 0) in vec3 aPos;
layout(location = 1) in vec3 aNormal;
layout(location = 2) in vec2 aUV;
uniform mat4 uMVP;
uniform mat4 uModel;
uniform mat3 uNormalMatrix;
out vec3 vNormal;
out vec3 vWorldPos;
out vec2 vUV;
void main() {
    vec4 wp = uModel * vec4(aPos, 1.0);
    vWorldPos = wp.xyz;
    vNormal = normalize(uNormalMatrix * aNormal);
    vUV = aUV;
    gl_Position = uMVP * vec4(aPos, 1.0);
}
