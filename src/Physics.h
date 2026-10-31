#pragma once
#include <glm/glm.hpp>
#include <vector>
#include <cmath>

struct Ball {
    glm::vec2 p{0.f, 4.5f}, v{0.f};
    float r = 0.12f;
    float sink = -1.f;
    bool moving() const { return glm::length(v) > 0.f; }
};

struct Box { glm::vec2 min, max; };

inline glm::vec2 rot(glm::vec2 v, float a) {
    float c = cosf(a), s = sinf(a);
    return {c * v.x + s * v.y, -s * v.x + c * v.y};
}

inline bool collideBox(Ball& b, const Box& w, float restitution = 0.8f) {
    glm::vec2 c = glm::clamp(b.p, w.min, w.max);
    glm::vec2 d = b.p - c;
    float dist = glm::length(d);
    if (dist >= b.r) return false;
    glm::vec2 n = dist > 1e-6f ? d / dist : glm::vec2(0, 1);
    b.p += n * (b.r - dist);
    float vn = glm::dot(b.v, n);
    if (vn < 0.f) b.v -= (1.f + restitution) * vn * n;
    return true;
}

struct Course {
    float W = 3.f, L = 6.f;
    glm::vec2 hole{0.f, -4.5f};
    float holeR = 0.2f;
    glm::vec2 bladeCenter{0.f, 0.f};
    Box bladeLocal{{-1.2f, -0.1f}, {1.2f, 0.1f}};
    float bladeSpeed = 1.2f;
    std::vector<Box> walls;

    Course() {
        float t = 0.3f;
        walls = {
            {{-W - t, -L - t}, {-W, L + t}},
            {{ W, -L - t},     { W + t, L + t}}, 
            {{-W, -L - t},     { W, -L}},        
            {{-W,  L},         { W, L + t}},     
        };
    }

    void step(Ball& b, float dt, float simTime) const {
        if (b.sink >= 0.f) { b.sink += dt; return; }
        b.p += b.v * dt;
        b.v *= expf(-0.8f * dt);                      
        if (glm::length(b.v) < 0.05f) b.v = glm::vec2(0.f);

        for (const Box& w : walls) collideBox(b, w);

        float a = bladeSpeed * simTime;
        Ball lb = b;
        lb.p = rot(b.p - bladeCenter, -a);
        lb.v = rot(b.v, -a);
        if (collideBox(lb, bladeLocal)) {
            b.p = rot(lb.p, a) + bladeCenter;
            b.v = rot(lb.v, a);
        }

        if (glm::length(b.p - hole) < holeR && glm::length(b.v) < 4.f) {
            b.sink = 0.f; b.v = glm::vec2(0.f); b.p = hole;
        }
    }
};