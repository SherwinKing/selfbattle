#include "ImageRenderer.hpp"

struct Point {
	GLfloat x;
	GLfloat y;
	GLfloat s;
	GLfloat t;
};

// Ref: base codes and https://gitlab.com/wikibooks-opengl/modern-tutorials/-/tree/master/text01_intro
ImageRenderer::ImageRenderer(uint32_t window_width, uint32_t window_height) {
	resize(window_width, window_height);

	// init freetype
	FT_Init_FreeType( &library );

	// Init shaders
	image_shader_program = gl_compile_program(
		//vertex shader:
		"#version 330\n"
		"in vec4 coord;\n"
		"out vec2 texpos;\n"
		"void main(void) {\n"
		"gl_Position = vec4(coord.xy, 0, 1);\n"
		"texpos = coord.zw;\n"
		// "printf('coord: %f %f %f %f', coord.x, coord.y, coord.z, coord.w);\n"
		// "printf('texpos: %f %f', texpos.x, texpos.y);\n"
		"}\n"
	,
		//fragment shader:
		"#version 330\n"
		"in vec2 texpos;\n"
		"uniform sampler2D tex;\n"
		// "uniform vec4 color;\n"
		"out vec4 fragColor;\n"
		"void main(void) {\n"
		// "fragColor = vec4(1, 1, 1, texture(tex, texpos).r) * color;\n"
		// "fragColor = vec4(texture(tex, texpos).rgba);\n"
		"fragColor = vec4(texture(tex, texpos).r, texture(tex, texpos).g, texture(tex, texpos).b, texture(tex, texpos).a);\n"
		"}\n"
	);

	// Look up the locations of vertex attributes:
	attribute_coord = glGetAttribLocation(image_shader_program, "coord");
	// Look up the locations of uniforms:
	uniform_tex = glGetUniformLocation(image_shader_program, "tex");
	// uniform_color = glGetUniformLocation(image_shader_program, "color");

	// Generate vbo
	glGenBuffers(1, &vbo);

	// Generate vertex array object
	glGenVertexArrays(1, &vertex_buffer_for_color_program);

	GL_ERRORS();

	// // Init FreeType2 library
	// if (FT_Init_FreeType(&ft)) {
	// 	fprintf(stderr, "Could not init freetype library
	// }
}

ImageRenderer::ImageRenderer()
 : ImageRenderer(1920, 1080){}

/**
 * Render image whose center is at (x, y)
 * 
 * Based on a tutorial and its corresponding codes:
 * https://en.wikibooks.org/wiki/OpenGL_Programming/Modern_OpenGL_Tutorial_Text_Rendering_01
 * https://gitlab.com/wikibooks-opengl/modern-tutorials/-/blob/master/text01_intro/text.cpp
 * Also used as reference: https://github.com/tangrams/harfbuzz-example/blob/master/src/hbshaper.h
 */
void ImageRenderer::render_image(const ImageData & image_data, float x, float y, float rotation_radians) {
	uint32_t image_width = image_data.size.x;
	uint32_t image_height = image_data.size.y;

	float render_left = x - image_width * sx / 2.0f;
	float render_bottom = y - image_height * sy / 2.0f;
	float render_right = x + image_width * sx / 2.0f;
	float render_top = y + image_height * sy / 2.0f;
	glm::vec4 bottom_left = glm::vec4(render_left, render_bottom, 0.0f, 1.0f);
	glm::vec4 bottom_right = glm::vec4(render_right, render_bottom, 1.0f, 1.0f);
	glm::vec4 top_left = glm::vec4(render_left, render_top, 1.0f, 1.0f);
	glm::vec4 top_right = glm::vec4(render_right, render_top, 0.0f, 1.0f);
	glm::vec4 center = glm::vec4(x, y, 0.0f, 1.0f);

	// based on https://gitlab.com/wikibooks-opengl/modern-tutorials/-/blob/master/text01_intro/text.cpp
	glUseProgram(image_shader_program);

	// Enable blending
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	// // Set the font color as uniform
	// glUniform4fv(uniform_color, 1, glm::vec4(1.0f, 1.0f, 1.0f, 1.0f));

	// Create a texture to hold each "glyph"
	GLuint tex;
	glActiveTexture(GL_TEXTURE0);
	glGenTextures(1, &tex);
	glBindTexture(GL_TEXTURE_2D, tex);
	glUniform1i(uniform_tex, 0);

	// Require 1 byte alignment when uploading texture data
	glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

	// Clamping to edges
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

	// Linear filtering
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

	// Set up the VBO for our vertex data
	glBindVertexArray(vertex_buffer_for_color_program);
	glEnableVertexAttribArray(attribute_coord);
	glBindBuffer(GL_ARRAY_BUFFER, vbo);
	glVertexAttribPointer(attribute_coord, 4, GL_FLOAT, GL_FALSE, 0, 0);


	// Upload the "bitmap", which contains an 32-bit rgba image
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, image_width, image_height, 0, GL_RGBA, GL_UNSIGNED_INT_8_8_8_8_REV, image_data.pixels.data());

	// Rotate the image to render
	if (rotation_radians != 0) {
		glm::mat4 center_rotation_mat = glm::translate(glm::mat4(1.0f), glm::vec3(center));
		center_rotation_mat = glm::scale(center_rotation_mat, glm::vec3(sx, sy, 1.0f));
		center_rotation_mat = glm::rotate(center_rotation_mat, rotation_radians, glm::vec3(0.0, 0.0, 1.0));
		center_rotation_mat = glm::scale(center_rotation_mat, glm::vec3(1.0f/sx, 1.0f/sy, 1.0f));
		center_rotation_mat = glm::translate(center_rotation_mat, glm::vec3(-center));

		bottom_left = center_rotation_mat * bottom_left;
		bottom_right = center_rotation_mat * bottom_right;
		top_right = center_rotation_mat * top_right;
		top_left = center_rotation_mat * top_left;
	}

	Point box[4] = {
		{bottom_left[0], bottom_left[1], 0, 0},
		{bottom_right[0], bottom_right[1], 1, 0},
		{top_left[0], top_left[1], 0, 1},
		{top_right[0], top_right[1], 1, 1},
	};

	// Draw the character on the screen
	glBufferData(GL_ARRAY_BUFFER, sizeof box, box, GL_DYNAMIC_DRAW);

	glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

	// Clean up
	glDisableVertexAttribArray(attribute_coord);
	glDeleteTextures(1, &tex);
	glBindTexture(GL_TEXTURE_2D, 0);
	glBindBuffer(GL_ARRAY_BUFFER, 0);

	GL_ERRORS();
}
