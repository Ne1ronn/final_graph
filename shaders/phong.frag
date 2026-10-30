#version 330 core
in vec3 FragPos;
in vec3 Normal;
in vec2 UV;
out vec4 FragColor;

uniform vec3 viewPos;
uniform vec3 ambient;
uniform vec3 sunDir;
uniform vec3 sunColor;
uniform vec3 lampPos;
uniform vec3 lampColor;
uniform int  lampOn;

uniform vec3  color;
uniform int   matType;
uniform float shininess;
uniform float specStrength;

float hash(vec2 p) { return fract(sin(dot(p, vec2(127.1, 311.7))) * 43758.5453); }
float noise(vec2 p) {
    vec2 i = floor(p), f = fract(p);
    f = f * f * (3.0 - 2.0 * f);
    return mix(mix(hash(i), hash(i + vec2(1, 0)), f.x),
               mix(hash(i + vec2(0, 1)), hash(i + vec2(1, 1)), f.x), f.y);
}

vec3 blinnPhong(vec3 L, vec3 lightColor, vec3 N, vec3 V, vec3 albedo) {
    float diff = max(dot(N, L), 0.0);
    vec3 H = normalize(L + V);
    float spec = pow(max(dot(N, H), 0.0), shininess) * specStrength;
    return (diff * albedo + spec) * lightColor;
}

void main() {
    vec3 albedo = color;

    if (matType == 1) {
        vec2 p = FragPos.xz;
        float n = noise(p * 30.0) * 0.6 + noise(p * 7.0) * 0.4;
        float stripe = step(0.5, fract(p.y * 1.5)) * 0.12;
        albedo = mix(vec3(0.10, 0.45, 0.12), vec3(0.25, 0.65, 0.20), n) - stripe;
    }
    if (matType == 2) { FragColor = vec4(color, 1.0); return; }

    vec3 N = normalize(Normal);
    vec3 V = normalize(viewPos - FragPos);

    vec3 result = ambient * albedo;
    result += blinnPhong(normalize(sunDir), sunColor, N, V, albedo);

    if (lampOn == 1) {
        vec3 d = lampPos - FragPos;
        float dist = length(d);
        float att = 1.0 / (1.0 + 0.35 * dist + 0.1 * dist * dist);
        result += blinnPhong(d / dist, lampColor, N, V, albedo) * att;
    }
    FragColor = vec4(result, 1.0);
}