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

struct Blade {
    glm::vec2 center;
    float halfLen;
    float speed;
};

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
    glm::vec2 start{0.f, 4.5f}, hole{0.f, -4.5f};
    float holeR = 0.2f;
    int par = 3;
    std::vector<Box> walls;
    std::vector<Blade> blades;

    static Course make(int level) {
        Course c;
        float t = 0.3f;
        c.walls = {
            {{-c.W - t, -c.L - t}, {-c.W, c.L + t}},
            {{ c.W, -c.L - t},     { c.W + t, c.L + t}},
            {{-c.W, -c.L - t},     { c.W, -c.L}},
            {{-c.W,  c.L},         { c.W, c.L + t}},
        };
        if (level == 0) {
            c.par = 3;
            c.blades = {{{0.f, 0.f}, 1.2f, 1.2f}};
        } else if (level == 1) {
            c.par = 4;
            c.start = {-2.f, 4.5f};
            c.hole = {2.f, -4.5f};
            c.walls.push_back({{-3.f, 1.5f}, {1.f, 1.9f}});
            c.walls.push_back({{-1.f, -1.9f}, {3.f, -1.5f}});
        } else {
            c.par = 4;
            c.blades = {{{-1.4f, 1.5f}, 1.3f, 1.5f}, {{1.4f, -1.5f}, 1.3f, -1.5f}};
            c.walls.push_back({{-0.35f, -0.35f}, {0.35f, 0.35f}});
        }
        return c;
    }

    void step(Ball& b, float dt, float t) const {
        if (b.sink >= 0.f) { b.sink += dt; return; }
        b.p += b.v * dt;
        b.v *= expf(-0.8f * dt);
        if (glm::length(b.v) < 0.05f) b.v = glm::vec2(0.f);

        for (const Box& w : walls) collideBox(b, w);

        for (const Blade& bl : blades) {
            float a = bl.speed * t;
            glm::vec2 q = rot(b.p - bl.center, -a);
            glm::vec2 vb = (rot(q, a + bl.speed * dt) - rot(q, a)) / dt;
            Ball lb = b;
            lb.p = q;
            lb.v = rot(b.v - vb, -a);
            Box box{{-bl.halfLen, -0.1f}, {bl.halfLen, 0.1f}};
            if (collideBox(lb, box)) {
                b.p = rot(lb.p, a) + bl.center;
                b.v = rot(lb.v, a) + vb;
            }
        }

        if (glm::length(b.p - hole) < holeR && glm::length(b.v) < 4.f) {
            b.sink = 0.f; b.v = glm::vec2(0.f); b.p = hole;
        }
    }
};

inline std::vector<glm::vec2> predictPath(const Course& c, Ball b, glm::vec2 v, float t,
                                          float horizon = 1.8f) {
    const float H = 1.f / 120.f;
    b.v = v;
    std::vector<glm::vec2> pts;
    int n = (int)(horizon / H);
    for (int i = 0; i < n; i++) {
        c.step(b, H, t);
        t += H;
        if (i % 8 == 7) pts.push_back(b.p);
        if (b.sink >= 0.f || !b.moving()) break;
    }
    return pts;
}