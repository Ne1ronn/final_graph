#pragma once
#include <glad/glad.h>
#include <vector>
#include <cstdint>
#include <cmath>

class Texture {
public:
    enum Kind { GRASS, WOOD };

    explicit Texture(Kind kind, int size = 512) {
        std::vector<uint8_t> px(size * size * 3);
        for (int y = 0; y < size; y++)
            for (int x = 0; x < size; x++) {
                float u = (float)x / size, v = (float)y / size;
                float r, g, b;
                if (kind == GRASS) {
                    float n = 0.5f * vnoise(u * 32, v * 32, 32) + 0.3f * vnoise(u * 8, v * 8, 8)
                            + 0.2f * vnoise(u * 64, v * 64, 64);
                    r = mix(0.10f, 0.28f, n); g = mix(0.42f, 0.68f, n); b = mix(0.10f, 0.20f, n);
                } else {
                    float grain = vnoise(u * 4, v * 64, 4) * 0.6f + vnoise(u * 16, v * 128, 16) * 0.4f;
                    float k = 0.65f + 0.5f * grain;
                    r = 0.55f * k; g = 0.34f * k; b = 0.18f * k;
                }
                uint8_t* p = &px[(y * size + x) * 3];
                p[0] = clamp8(r); p[1] = clamp8(g); p[2] = clamp8(b);
            }
        glGenTextures(1, &id);
        glBindTexture(GL_TEXTURE_2D, id);
        glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, size, size, 0, GL_RGB, GL_UNSIGNED_BYTE, px.data());
        glGenerateMipmap(GL_TEXTURE_2D);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    }
    Texture(const Texture&) = delete;
    ~Texture() { glDeleteTextures(1, &id); }

    void bind(int unit) const {
        glActiveTexture(GL_TEXTURE0 + unit);
        glBindTexture(GL_TEXTURE_2D, id);
    }

private:
    GLuint id = 0;

    static float mix(float a, float b, float t) { return a + (b - a) * t; }
    static uint8_t clamp8(float f) { return (uint8_t)(f < 0 ? 0 : f > 1 ? 255 : f * 255.f); }
    static float hash(int x, int y) {
        unsigned n = (unsigned)x * 374761393u + (unsigned)y * 668265263u;
        n = (n ^ (n >> 13)) * 1274126177u;
        return ((n ^ (n >> 16)) & 0xffff) / 65535.f;
    }
    static float vnoise(float x, float y, int per) {
        int xi = (int)floorf(x), yi = (int)floorf(y);
        float fx = x - xi, fy = y - yi;
        fx = fx * fx * (3 - 2 * fx); fy = fy * fy * (3 - 2 * fy);
        int x0 = ((xi % per) + per) % per, x1 = (x0 + 1) % per;
        int y0 = ((yi % per) + per) % per, y1 = (y0 + 1) % per;
        return mix(mix(hash(x0, y0), hash(x1, y0), fx), mix(hash(x0, y1), hash(x1, y1), fx), fy);
    }
};