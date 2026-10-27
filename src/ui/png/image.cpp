#include "image.hpp"
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

namespace ui::image {
    bool LoadTextureFromMemory(const uint8_t* data, size_t data_size, GLuint* out_texture, int* out_width, int* out_height) {
        int w, h, ch;
        unsigned char* pixels = stbi_load_from_memory(data, data_size, &w, &h, &ch, 4);
        if (!pixels) return false;

        GLuint texture;
        glGenTextures(1, &texture);
        glBindTexture(GL_TEXTURE_2D, texture);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, pixels);
        stbi_image_free(pixels);

        *out_texture = texture;
        *out_width = w;
        *out_height = h;
        return true;
    }
}
