#include "TextParagraph.hpp"

TextParagraph::TextParagraph(std::string text, glm::vec4 color, uint32_t font_size) :
    text(text),
    font_size(font_size),
    color(color) {}

float TextParagraph::render(float x, float y, TextRenderer & text_renderer) {
    if (text.length() == 0) {
        return y;
    }
    RenderResult result = text_renderer.render_text(text, x, y, color, font_size);
    if (result.remaining_text == text) {
        throw std::runtime_error("TextParagraph::render: The start position is out of allowed boundary.");
    }
    while (result.remaining_text.length() > 0) {
        y -= text_renderer.get_line_height_by_font_size(font_size);
        result = text_renderer.render_text(result.remaining_text, x, y, color, font_size);
    }
    return y;
}