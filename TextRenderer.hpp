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

typedef struct RenderResult {
	float cusor_ending_x;
	float cusor_ending_y;
	std::string remaining_text;
} RenderResult;

class TextRenderer {
private:
	std::string font_file_path;
	FT_Library library;
	GLuint text_shader_program;
	GLint attribute_coord;
	GLint uniform_tex;
	GLint uniform_color;
	GLuint vbo;
	GLuint vertex_buffer_for_color_program;

	float sx;
	float sy;

public:
	TextRenderer();
	TextRenderer(uint32_t window_width, uint32_t window_height);
	TextRenderer(std::string font_file_path_from_dist);
	TextRenderer(uint32_t window_width, uint32_t window_height, std::string font_file_path_from_dist);

	inline void resize(uint32_t window_width, uint32_t window_height) {
		sx = 2.0f / window_width;
		sy = 2.0f / window_height;
	}

	inline float get_line_height_by_font_size(uint32_t font_size) {
		return sy * font_size * 1.2f;
	}

	/**
	 * Render text using the currently loaded font and currently set font size.
	 * Rendering starts at coordinates (x, y), z is always 0.
	 * The range of the pixel coordinates (x, y) is (-1, -1) to (1, 1).
	 * The pixel coordinates that the FreeType2 library uses are scaled by (sx, sy).
	 * 
	 * Based on a tutorial and its corresponding codes:
	 * https://en.wikibooks.org/wiki/OpenGL_Programming/Modern_OpenGL_Tutorial_Text_Rendering_01
	 * https://gitlab.com/wikibooks-opengl/modern-tutorials/-/blob/master/text01_intro/text.cpp
	 * Also used as reference: https://github.com/tangrams/harfbuzz-example/blob/master/src/hbshaper.h
	 */
	RenderResult render_text(const std::string text, float x, float y,
		glm::vec4 color  = glm::vec4(0.0f, 0.0f, 0.0f, 1.0f),
		uint32_t font_size = 36);
};
