#pragma once
#include <GLES2/gl2.h> 

namespace ui::image {
    bool LoadTextureFromMemory(const uint8_t* data, size_t data_size, GLuint* out_texture, int* out_width, int* out_height);
}
