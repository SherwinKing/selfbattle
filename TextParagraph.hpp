#pragma once
#include "TextRenderer.hpp"

class TextParagraph {
private:
    std::string text;
    uint32_t font_size;
    glm::vec4 color;

public:
    TextParagraph(std::string text, 
        glm::vec4 color  = glm::vec4(0.0f, 0.0f, 0.0f, 1.0f),
		uint32_t font_size = 30);

    /**
     * Render the text paragraph at the given position.
     * The range of the pixel coordinates (x, y) is (-1, -1) to (1, 1).
     * @param x The x coordinate of the bottom left corner.
     * @param y The y coordinate of the bottom left corner.
     * @return The y coordinate of the cursor after rendering.
     */
    float render(float x, float y, TextRenderer & text_renderer);
};