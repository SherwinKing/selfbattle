#include "TextRenderer.hpp"

struct Point {
	GLfloat x;
	GLfloat y;
	GLfloat s;
	GLfloat t;
};

// Ref: base codes and https://gitlab.com/wikibooks-opengl/modern-tutorials/-/tree/master/text01_intro
TextRenderer::TextRenderer(uint32_t window_width, uint32_t window_height, std::string font_file_path_from_dist) {
	resize(window_width, window_height);

	this->font_file_path = data_path(font_file_path_from_dist);
	// init freetype
	FT_Init_FreeType( &library );

	// Init shaders
	text_shader_program = gl_compile_program(
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
		"uniform vec4 color;\n"
		"out vec4 fragColor;\n"
		"void main(void) {\n"
		"fragColor = vec4(1, 1, 1, texture(tex, texpos).r) * color;\n"
		"}\n"
	);

	// Look up the locations of vertex attributes:
	attribute_coord = glGetAttribLocation(text_shader_program, "coord");
	// Look up the locations of uniforms:
	uniform_tex = glGetUniformLocation(text_shader_program, "tex");
	uniform_color = glGetUniformLocation(text_shader_program, "color");

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

TextRenderer::TextRenderer(std::string font_file_path_from_dist)
 : TextRenderer(1920, 1080, font_file_path_from_dist){}

TextRenderer::TextRenderer(uint32_t window_width, uint32_t window_height)
 : TextRenderer(window_width, window_height, "font/Roboto/Roboto-Regular.ttf"){}

TextRenderer::TextRenderer()
 : TextRenderer(1920, 1080, "font/Roboto/Roboto-Regular.ttf"){}

/**
 * Render text using the currently loaded font and currently set font size.
 * Rendering starts at coordinates (x, y), z is always 0.
 * The pixel coordinates that the FreeType2 library uses are scaled by (sx, sy).
 * 
 * Based on a tutorial and its corresponding codes:
 * https://en.wikibooks.org/wiki/OpenGL_Programming/Modern_OpenGL_Tutorial_Text_Rendering_01
 * https://gitlab.com/wikibooks-opengl/modern-tutorials/-/blob/master/text01_intro/text.cpp
 * Also used as reference: https://github.com/tangrams/harfbuzz-example/blob/master/src/hbshaper.h
 */
RenderResult TextRenderer::render_text(std::string text, float x, float y, glm::vec4 color, uint32_t font_size) {
	FT_Face ft_face = nullptr;
	FT_New_Face(library, font_file_path.c_str(), 0, &ft_face);

	FT_Set_Pixel_Sizes(ft_face, 0, font_size);

	hb_font_t *font = hb_ft_font_create(ft_face, NULL);

	hb_buffer_t *buf = hb_buffer_create();

	hb_buffer_add_utf8(buf, text.c_str(), -1, 0, -1);
	hb_buffer_guess_segment_properties(buf);

	hb_shape(font, buf, NULL, 0);

	unsigned int len = hb_buffer_get_length(buf);
	hb_glyph_info_t *info = hb_buffer_get_glyph_infos(buf, NULL);

	// based on https://gitlab.com/wikibooks-opengl/modern-tutorials/-/blob/master/text01_intro/text.cpp
	glUseProgram(text_shader_program);

	// Enable blending
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	// Set the font color as uniform
	GLfloat color_glfloat[4] = { color.r, color.g, color.b, color.a };
	glUniform4fv(uniform_color, 1, color_glfloat);


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

	std::string remaining_text;
	// based on: https://stackoverflow.com/questions/236129/how-do-i-iterate-over-the-words-of-a-string
	std::istringstream iss(text);
	std::vector<std::string> words{std::istream_iterator<std::string>{iss},
							std::istream_iterator<std::string>{}};


	// Render characters
	// https://github.com/tangrams/harfbuzz-example/blob/master/src/hbshaper.h
	uint32_t char_id = 0;
	uint32_t next_word_id = 0;
	uint32_t next_word_id_increment;
	while (char_id < len) {
	// for (unsigned int i = 0; i < len; i++) {
		// Check if next char is a space
		uint32_t render_char_num;
		if (text[char_id] == ' ') {
			render_char_num = 1;
			next_word_id_increment = 0;
		} else {
			render_char_num = words[next_word_id].length();
			next_word_id_increment = 1;
		}

		// // Render the word

		for (uint32_t i = char_id; i < char_id + render_char_num; i++) {

			FT_Load_Glyph(ft_face, info[i].codepoint, FT_LOAD_DEFAULT);
			FT_GlyphSlot & glyph_slot = ft_face->glyph;
			FT_Render_Glyph(glyph_slot, FT_RENDER_MODE_NORMAL);
			FT_Bitmap & ftBitmap = glyph_slot->bitmap;


			// Upload the "bitmap", which contains an 8-bit grayscale image, as an RED texture
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RED, ftBitmap.width, ftBitmap.rows, 0, GL_RED, GL_UNSIGNED_BYTE, ftBitmap.buffer);

			// Calculate the vertex and texture coordinates
			float x2 = x + glyph_slot->bitmap_left * sx;
			float y2 = -y - glyph_slot->bitmap_top * sy;
			float w = ftBitmap.width * sx;
			float h = ftBitmap.rows * sy;

			// Each point represents a corner of the texture: (vetex x, vertex y, texture x, texture y)
			Point box[4] = {
				{x2, -y2, 0, 0},
				{x2 + w, -y2, 1, 0},
				{x2, -y2 - h, 0, 1},
				{x2 + w, -y2 - h, 1, 1},
			};

			// Draw the character on the screen
			glBufferData(GL_ARRAY_BUFFER, sizeof box, box, GL_DYNAMIC_DRAW);
			glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

			// Advance the cursor to the start of the next character
			x += (glyph_slot->advance.x >> 6) * sx;
			y += (glyph_slot->advance.y >> 6) * sy;
		}

		// increment the index
		char_id+=render_char_num;
		next_word_id+=next_word_id_increment;

		// Stop rendering if the cursor is about to cross the screen boundary
		if (x > 0.8 && char_id < len && text[char_id] != ' ') {
			remaining_text = text.substr(char_id, len);
			break;
		}
	}

	// Clean up
	glDisableVertexAttribArray(attribute_coord);
	glDeleteTextures(1, &tex);
	glBindTexture(GL_TEXTURE_2D, 0);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	hb_buffer_destroy(buf);

	GL_ERRORS();

	RenderResult result = { x, y, remaining_text};
	return result;
}
