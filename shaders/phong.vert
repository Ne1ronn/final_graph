#version 330 core
layout(location = 0) in vec3 aPos;
layout(location = 1) in vec3 aNormal;
layout(location = 2) in vec2 aUV;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;
uniform mat3 normalMat;

out vec3 FragPos;
out vec3 Normal;
out vec2 UV;

void main() {
    vec4 world = model * vec4(aPos, 1.0);
    FragPos = world.xyz;
    Normal  = normalMat * aNormal;
    UV      = aUV;
    gl_Position = projection * view * world;
}