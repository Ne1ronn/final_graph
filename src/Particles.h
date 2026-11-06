#pragma once
#include <glm/glm.hpp>
#include <vector>
#include <random>
#include <algorithm>
#include <cmath>

struct Particle { glm::vec3 p, v, color; float life; };

class Confetti {
public:
    std::vector<Particle> ps;

    void burst(glm::vec3 origin, int n = 80) {
        static std::mt19937 rng{12345};
        std::uniform_real_distribution<float> u(0.f, 1.f);
        static const glm::vec3 pal[5] = {{1, .2f, .2f}, {1, .85f, .1f}, {.2f, .8f, .3f}, {.2f, .5f, 1}, {.9f, .3f, .9f}};
        for (int i = 0; i < n; i++) {
            float a = u(rng) * 6.2831853f, s = 1.f + u(rng) * 2.5f;
            ps.push_back({origin, glm::vec3(cosf(a) * s, 3.f + u(rng) * 3.f, sinf(a) * s), pal[i % 5], 2.5f + u(rng)});
        }
    }
    void update(float dt) {
        for (auto& p : ps) {
            p.v.y -= 9.f * dt;
            p.p += p.v * dt;
            if (p.p.y < 0.02f) { p.p.y = 0.02f; p.v = glm::vec3(0.f); }
            p.life -= dt;
        }
        ps.erase(std::remove_if(ps.begin(), ps.end(), [](const Particle& p) { return p.life <= 0.f; }), ps.end());
    }
    void clear() { ps.clear(); }
};