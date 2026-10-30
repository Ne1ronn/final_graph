#pragma once
#include <glad/glad.h>
#include <glm/glm.hpp>
#include <vector>
#include <cmath>
#include <cstddef>

struct Vertex { glm::vec3 pos; glm::vec3 normal; glm::vec2 uv; };

class Mesh {
public:
    Mesh(const std::vector<Vertex>& v, const std::vector<unsigned>& idx) {
        indexCount = (GLsizei)idx.size();
        glGenVertexArrays(1, &vao);
        glGenBuffers(1, &vbo);
        glGenBuffers(1, &ebo);
        glBindVertexArray(vao);
        glBindBuffer(GL_ARRAY_BUFFER, vbo);
        glBufferData(GL_ARRAY_BUFFER, v.size() * sizeof(Vertex), v.data(), GL_STATIC_DRAW);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, idx.size() * sizeof(unsigned), idx.data(), GL_STATIC_DRAW);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, pos));
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, normal));
        glEnableVertexAttribArray(2);
        glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, uv));
        glBindVertexArray(0);
    }
    Mesh(const Mesh&) = delete;
    ~Mesh() { glDeleteVertexArrays(1, &vao); glDeleteBuffers(1, &vbo); glDeleteBuffers(1, &ebo); }

    void draw() const {
        glBindVertexArray(vao);
        glDrawElements(GL_TRIANGLES, indexCount, GL_UNSIGNED_INT, 0);
    }

    static Mesh cube() {
        struct Face { glm::vec3 n, u, v; };
        const Face faces[6] = {
            {{0,0,1},  {1,0,0},  {0,1,0}},  {{0,0,-1}, {-1,0,0}, {0,1,0}},
            {{1,0,0},  {0,0,-1}, {0,1,0}},  {{-1,0,0}, {0,0,1},  {0,1,0}},
            {{0,1,0},  {1,0,0},  {0,0,-1}}, {{0,-1,0}, {1,0,0},  {0,0,1}}};
        const float cs[4][2] = {{-1,-1},{1,-1},{1,1},{-1,1}};
        std::vector<Vertex> v; std::vector<unsigned> idx;
        for (const Face& f : faces) {
            unsigned base = (unsigned)v.size();
            for (auto& c : cs)
                v.push_back({0.5f * (f.n + c[0] * f.u + c[1] * f.v), f.n, {c[0] * .5f + .5f, c[1] * .5f + .5f}});
            idx.insert(idx.end(), {base, base+1, base+2, base, base+2, base+3});
        }
        return Mesh(v, idx);
    }

    static Mesh plane() {
        std::vector<Vertex> v = {
            {{-.5f,0, .5f},{0,1,0},{0,0}}, {{ .5f,0, .5f},{0,1,0},{1,0}},
            {{ .5f,0,-.5f},{0,1,0},{1,1}}, {{-.5f,0,-.5f},{0,1,0},{0,1}}};
        return Mesh(v, {0,1,2, 0,2,3});
    }

    static Mesh sphere(int stacks = 24, int sectors = 32) {
        const float PI = 3.14159265f;
        std::vector<Vertex> v; std::vector<unsigned> idx;
        for (int i = 0; i <= stacks; i++) {
            float th = PI * i / stacks;
            for (int j = 0; j <= sectors; j++) {
                float ph = 2 * PI * j / sectors;
                glm::vec3 p(sinf(th) * cosf(ph), cosf(th), sinf(th) * sinf(ph));
                v.push_back({p, p, {(float)j / sectors, (float)i / stacks}});
            }
        }
        for (int i = 0; i < stacks; i++)
            for (int j = 0; j < sectors; j++) {
                unsigned a = i * (sectors + 1) + j, b = a + sectors + 1;
                idx.insert(idx.end(), {a, a+1, b, a+1, b+1, b});
            }
        return Mesh(v, idx);
    }

    static Mesh cylinder(int n = 32) {
        const float PI = 3.14159265f;
        std::vector<Vertex> v; std::vector<unsigned> idx;
        for (int j = 0; j <= n; j++) {
            float p = 2 * PI * j / n, c = cosf(p), s = sinf(p);
            v.push_back({{c,  .5f, s}, {c,0,s}, {(float)j / n, 1}});
            v.push_back({{c, -.5f, s}, {c,0,s}, {(float)j / n, 0}});
        }
        for (int j = 0; j < n; j++) {
            unsigned t = 2*j, b = 2*j+1, t2 = 2*j+2, b2 = 2*j+3;
            idx.insert(idx.end(), {t, t2, b, t2, b2, b});
        }
        for (int cap = 0; cap < 2; cap++) {
            float y = cap == 0 ? .5f : -.5f, ny = cap == 0 ? 1.f : -1.f;
            unsigned center = (unsigned)v.size();
            v.push_back({{0, y, 0}, {0, ny, 0}, {.5f, .5f}});
            unsigned ring = (unsigned)v.size();
            for (int j = 0; j <= n; j++) {
                float p = 2 * PI * j / n, c = cosf(p), s = sinf(p);
                v.push_back({{c, y, s}, {0, ny, 0}, {c * .5f + .5f, s * .5f + .5f}});
            }
            for (int j = 0; j < n; j++) {
                if (cap == 0) idx.insert(idx.end(), {center, ring + j + 1, ring + j});
                else          idx.insert(idx.end(), {center, ring + j, ring + j + 1});
            }
        }
        return Mesh(v, idx);
    }

private:
    GLuint vao = 0, vbo = 0, ebo = 0;
    GLsizei indexCount = 0;
};