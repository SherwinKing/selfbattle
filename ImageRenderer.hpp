#pragma once

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <vector>

// #include <GL/glew.h>
// #include <GL/freeglut.h>

#include "GL.hpp"
#include "gl_compile_program.hpp"
#include "gl_errors.hpp"

#define GLM_FORCE_RADIANS
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

// #include <ft2build.h>

#include <ft2build.h>
#include FT_FREETYPE_H

#include <hb.h>
#include <hb-ft.h>

#include <iostream>
#include <string>
#include <sstream>
#include <iterator>
#include <tuple>

#include "data_path.hpp"

typedef struct ImageData {
	std::vector<glm::u8vec4> pixels;
	glm::uvec2 size;
	uint8_t sprite_index;
} ImageData;

class ImageRenderer {
private:
	std::string font_file_path;
	FT_Library library;
	GLuint image_shader_program;
	GLint attribute_coord;
	GLint uniform_tex;
	// GLint uniform_color;
	GLuint vbo;
	GLuint vertex_buffer_for_color_program;

	float sx;
	float sy;

public:
	ImageRenderer();
	ImageRenderer(uint32_t window_width, uint32_t window_height);

	inline void resize(uint32_t window_width, uint32_t window_height) {
		sx = 2.0f / window_width;
		sy = 2.0f / window_height;
	}

	void render_image(const ImageData & image_data, float x, float y, float rotation_degrees = 0.0f);
	// void render_image(const std::vector<glm::u8vec4> &image, glm::uvec2 size, float x, float y);
};
