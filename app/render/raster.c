/* Extracted from Ashtray-Archer/utilities-android-phone-user
 * ec022aea6fe6836ea78f479b498e85b3452d88b7, accelerometer native_main.c.
 * Canvas replaces the Android window buffer; app-specific sensor code removed.
 */
#include "raster.h"
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
struct glyph {
    char character;
    uint8_t rows[7];
};

static const struct glyph glyphs[] = {
    {' ', {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}},
    {'+', {0x00, 0x04, 0x04, 0x1f, 0x04, 0x04, 0x00}},
    {'-', {0x00, 0x00, 0x00, 0x1f, 0x00, 0x00, 0x00}},
    {'.', {0x00, 0x00, 0x00, 0x00, 0x00, 0x0c, 0x0c}},
    {'/', {0x01, 0x02, 0x02, 0x04, 0x08, 0x08, 0x10}},
    {'0', {0x0e, 0x11, 0x13, 0x15, 0x19, 0x11, 0x0e}},
    {'1', {0x04, 0x0c, 0x14, 0x04, 0x04, 0x04, 0x1f}},
    {'2', {0x0e, 0x11, 0x01, 0x02, 0x04, 0x08, 0x1f}},
    {'3', {0x1e, 0x01, 0x01, 0x0e, 0x01, 0x01, 0x1e}},
    {'4', {0x02, 0x06, 0x0a, 0x12, 0x1f, 0x02, 0x02}},
    {'5', {0x1f, 0x10, 0x10, 0x1e, 0x01, 0x01, 0x1e}},
    {'6', {0x0e, 0x10, 0x10, 0x1e, 0x11, 0x11, 0x0e}},
    {'7', {0x1f, 0x01, 0x02, 0x04, 0x08, 0x08, 0x08}},
    {'8', {0x0e, 0x11, 0x11, 0x0e, 0x11, 0x11, 0x0e}},
    {'9', {0x0e, 0x11, 0x11, 0x0f, 0x01, 0x01, 0x0e}},
    {'A', {0x0e, 0x11, 0x11, 0x1f, 0x11, 0x11, 0x11}},
    {'C', {0x0f, 0x10, 0x10, 0x10, 0x10, 0x10, 0x0f}},
    {'D', {0x1e, 0x11, 0x11, 0x11, 0x11, 0x11, 0x1e}},
    {'E', {0x1f, 0x10, 0x10, 0x1e, 0x10, 0x10, 0x1f}},
    {'G', {0x0f, 0x10, 0x10, 0x17, 0x11, 0x11, 0x0f}},
    {'H', {0x11, 0x11, 0x11, 0x1f, 0x11, 0x11, 0x11}},
    {'I', {0x1f, 0x04, 0x04, 0x04, 0x04, 0x04, 0x1f}},
    {'L', {0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x1f}},
    {'M', {0x11, 0x1b, 0x15, 0x15, 0x11, 0x11, 0x11}},
    {'a', {0x00, 0x00, 0x0e, 0x01, 0x0f, 0x11, 0x0f}},
    {'d', {0x01, 0x01, 0x0f, 0x11, 0x11, 0x11, 0x0f}},
    {'e', {0x00, 0x00, 0x0e, 0x11, 0x1f, 0x10, 0x0e}},
    {'g', {0x00, 0x00, 0x0f, 0x11, 0x0f, 0x01, 0x0e}},
    {'h', {0x10, 0x10, 0x1e, 0x11, 0x11, 0x11, 0x11}},
    {'i', {0x04, 0x00, 0x0c, 0x04, 0x04, 0x04, 0x0e}},
    {'m', {0x00, 0x00, 0x1a, 0x15, 0x15, 0x15, 0x15}},
    {'n', {0x00, 0x00, 0x1e, 0x11, 0x11, 0x11, 0x11}},
    {'o', {0x00, 0x00, 0x0e, 0x11, 0x11, 0x11, 0x0e}},
    {'p', {0x00, 0x00, 0x1e, 0x11, 0x1e, 0x10, 0x10}},
    {'r', {0x00, 0x00, 0x16, 0x19, 0x10, 0x10, 0x10}},
    {'s', {0x00, 0x00, 0x0f, 0x10, 0x0e, 0x01, 0x1e}},
    {'t', {0x04, 0x04, 0x1f, 0x04, 0x04, 0x04, 0x03}},
    {'u', {0x00, 0x00, 0x11, 0x11, 0x11, 0x13, 0x0d}},
    {'y', {0x00, 0x00, 0x11, 0x11, 0x0f, 0x01, 0x0e}},
    {'b', {0x10,0x10,0x1e,0x11,0x11,0x11,0x1e}},
    {'c', {0x00,0x00,0x0f,0x10,0x10,0x10,0x0f}},
    {'f', {0x06,0x08,0x1e,0x08,0x08,0x08,0x08}},
    {'j', {0x02,0x00,0x06,0x02,0x02,0x12,0x0c}},
    {'k', {0x10,0x10,0x12,0x14,0x18,0x14,0x12}},
    {'l', {0x0c,0x04,0x04,0x04,0x04,0x04,0x0e}},
    {'q', {0x00,0x00,0x0f,0x11,0x0f,0x01,0x01}},
    {'v', {0x00,0x00,0x11,0x11,0x11,0x0a,0x04}},
    {'w', {0x00,0x00,0x11,0x11,0x15,0x15,0x0a}},
    {'x', {0x00,0x00,0x11,0x0a,0x04,0x0a,0x11}},
    {'z', {0x00,0x00,0x1f,0x02,0x04,0x08,0x1f}},
    {'N', {0x11, 0x19, 0x19, 0x15, 0x13, 0x13, 0x11}},
    {'O', {0x0e, 0x11, 0x11, 0x11, 0x11, 0x11, 0x0e}},
    {'P', {0x1e, 0x11, 0x11, 0x1e, 0x10, 0x10, 0x10}},
    {'R', {0x1e, 0x11, 0x11, 0x1e, 0x14, 0x12, 0x11}},
    {'S', {0x0f, 0x10, 0x10, 0x0e, 0x01, 0x01, 0x1e}},
    {'T', {0x1f, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04}},
    {'U', {0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x0e}},
    {'X', {0x11, 0x11, 0x0a, 0x04, 0x0a, 0x11, 0x11}},
    {'Y', {0x11, 0x11, 0x0a, 0x04, 0x04, 0x04, 0x04}},
    {'Z', {0x1f, 0x01, 0x02, 0x04, 0x08, 0x10, 0x1f}},
    {'B', {0x1e,0x11,0x11,0x1e,0x11,0x11,0x1e}},
    {'F', {0x1f,0x10,0x10,0x1e,0x10,0x10,0x10}},
    {'J', {0x07,0x02,0x02,0x02,0x12,0x12,0x0c}},
    {'K', {0x11,0x12,0x14,0x18,0x14,0x12,0x11}},
    {'Q', {0x0e,0x11,0x11,0x11,0x15,0x12,0x0d}},
    {'V', {0x11,0x11,0x11,0x11,0x11,0x0a,0x04}},
    {'W', {0x11,0x11,0x11,0x15,0x15,0x1b,0x11}},
    {',', {0,0,0,0,0,4,8}}, {':', {0,4,4,0,4,4,0}},
    {';', {0,4,4,0,4,4,8}}, {'[', {14,8,8,8,8,8,14}},
    {']', {14,2,2,2,2,2,14}}, {'(', {2,4,8,8,8,4,2}},
    {')', {8,4,2,2,2,4,8}}, {'=', {0,0,31,0,31,0,0}},
    {'|', {4,4,4,4,4,4,4}}, {'^', {4,10,17,0,0,0,0}},
    {'<', {1,2,4,8,4,2,1}}, {'>', {16,8,4,2,4,8,16}},
    {'_', {0,0,0,0,0,0,31}}, {'*', {0,21,14,31,14,21,0}},
    {'\x7f', {0x04,0x04,0x0a,0x0a,0x11,0x11,0x11}},
    {'?', {0x0e, 0x11, 0x01, 0x02, 0x04, 0x00, 0x04}},
};


static const uint8_t *glyph_rows(char character)
{
    size_t count = sizeof(glyphs) / sizeof(glyphs[0]);
    for (size_t index = 0; index < count; ++index) {
        if (glyphs[index].character == character) {
            return glyphs[index].rows;
        }
    }
    return glyphs[count - 1U].rows;
}

static void put_pixel(Canvas *buffer, int32_t x, int32_t y, uint32_t value)
{
    if (x < 0 || y < 0 || x >= buffer->width || y >= buffer->height || y < buffer->clip_top || y >= buffer->clip_bottom) {
        return;
    }
    uint32_t *pixels = (uint32_t *)buffer->bits;
    size_t offset = (size_t)y * (size_t)buffer->stride + (size_t)x;
    pixels[offset] = value;
}

static void blend_pixel(
    Canvas *buffer,
    int32_t x,
    int32_t y,
    uint32_t value,
    uint8_t coverage)
{
    if (x < 0 || y < 0 || x >= buffer->width || y >= buffer->height || y < buffer->clip_top || y >= buffer->clip_bottom || coverage == 0U) {
        return;
    }

    uint32_t *pixels = (uint32_t *)buffer->bits;
    size_t offset = (size_t)y * (size_t)buffer->stride + (size_t)x;
    if (coverage == 255U) {
        pixels[offset] = value;
        return;
    }

    uint32_t under = pixels[offset];
    uint32_t inverse = 255U - (uint32_t)coverage;
    uint32_t mixed = 0U;
    for (unsigned int shift = 0U; shift < 32U; shift += 8U) {
        uint32_t under_channel = (under >> shift) & 0xffU;
        uint32_t value_channel = (value >> shift) & 0xffU;
        uint32_t channel =
            (under_channel * inverse + value_channel * (uint32_t)coverage + 127U) / 255U;
        mixed |= channel << shift;
    }
    pixels[offset] = mixed;
}

static bool glyph_cell_on(const uint8_t *rows, int32_t row, int32_t column)
{
    if (row < 0 || row >= 7 || column < 0 || column >= 5) {
        return false;
    }
    uint8_t mask = (uint8_t)(1U << (unsigned int)(4 - column));
    return (rows[row] & mask) != 0U;
}

#define GLYPH_SUPERSAMPLE 4
#define GLYPH_CACHE_CAPACITY 96

struct glyph_mask_cache_entry {
    char character;
    int32_t scale;
    uint8_t *coverage;
};

static struct glyph_mask_cache_entry glyph_mask_cache[GLYPH_CACHE_CAPACITY];
static size_t glyph_cache_replacement = 0U;

static int32_t floor_divide(int32_t numerator, int32_t denominator)
{
    if (numerator >= 0) {
        return numerator / denominator;
    }
    return -((-numerator + denominator - 1) / denominator);
}

/*
 * Reconstruct one continuous glyph from the 5x7 samples, then sample that
 * shape on a 4x4 grid inside each framebuffer pixel. The interpolation
 * threshold is exactly 2/5: low enough to bridge diagonal bitmap samples
 * such as X without turning the one-cell strokes into solid blocks.
 */
static bool glyph_subsample_on(
    const uint8_t *rows,
    int32_t pixel_x,
    int32_t pixel_y,
    int32_t sub_x,
    int32_t sub_y,
    int32_t scale)
{
    int32_t denominator = 8 * scale;
    int32_t x_numerator =
        8 * pixel_x + 2 * sub_x + 1 - 4 * scale;
    int32_t y_numerator =
        8 * pixel_y + 2 * sub_y + 1 - 4 * scale;

    int32_t column = floor_divide(x_numerator, denominator);
    int32_t row = floor_divide(y_numerator, denominator);
    int32_t x_remainder = x_numerator - column * denominator;
    int32_t y_remainder = y_numerator - row * denominator;

    int32_t x0_weight = denominator - x_remainder;
    int32_t x1_weight = x_remainder;
    int32_t y0_weight = denominator - y_remainder;
    int32_t y1_weight = y_remainder;

    int32_t weighted = 0;
    if (glyph_cell_on(rows, row, column)) {
        weighted += x0_weight * y0_weight;
    }
    if (glyph_cell_on(rows, row, column + 1)) {
        weighted += x1_weight * y0_weight;
    }
    if (glyph_cell_on(rows, row + 1, column)) {
        weighted += x0_weight * y1_weight;
    }
    if (glyph_cell_on(rows, row + 1, column + 1)) {
        weighted += x1_weight * y1_weight;
    }

    int32_t full_weight = denominator * denominator;
    return 5 * weighted >= 2 * full_weight;
}

static uint8_t glyph_pixel_coverage(
    const uint8_t *rows,
    int32_t pixel_x,
    int32_t pixel_y,
    int32_t scale)
{
    int32_t covered = 0;
    for (int32_t sub_y = 0; sub_y < GLYPH_SUPERSAMPLE; ++sub_y) {
        for (int32_t sub_x = 0; sub_x < GLYPH_SUPERSAMPLE; ++sub_x) {
            if (glyph_subsample_on(
                    rows,
                    pixel_x,
                    pixel_y,
                    sub_x,
                    sub_y,
                    scale)) {
                covered += 1;
            }
        }
    }
    return (uint8_t)((covered * 255 + 8) / 16);
}

static uint8_t *build_glyph_mask(char character, int32_t scale)
{
    int32_t width = 5 * scale;
    int32_t height = 7 * scale;
    size_t size = (size_t)width * (size_t)height;
    uint8_t *coverage = (uint8_t *)malloc(size);
    if (coverage == NULL) {
        return NULL;
    }

    const uint8_t *rows = glyph_rows(character);
    for (int32_t y = 0; y < height; ++y) {
        for (int32_t x = 0; x < width; ++x) {
            coverage[(size_t)y * (size_t)width + (size_t)x] =
                glyph_pixel_coverage(rows, x, y, scale);
        }
    }
    return coverage;
}

static const uint8_t *cached_glyph_mask(char character, int32_t scale)
{
    size_t empty = GLYPH_CACHE_CAPACITY;
    for (size_t index = 0U; index < GLYPH_CACHE_CAPACITY; ++index) {
        if (glyph_mask_cache[index].coverage == NULL) {
            if (empty == GLYPH_CACHE_CAPACITY) {
                empty = index;
            }
            continue;
        }
        if (glyph_mask_cache[index].character == character &&
            glyph_mask_cache[index].scale == scale) {
            return glyph_mask_cache[index].coverage;
        }
    }

    size_t slot = empty;
    if (slot == GLYPH_CACHE_CAPACITY) {
        slot = glyph_cache_replacement;
        glyph_cache_replacement =
            (glyph_cache_replacement + 1U) % GLYPH_CACHE_CAPACITY;
        free(glyph_mask_cache[slot].coverage);
        glyph_mask_cache[slot].coverage = NULL;
    }

    uint8_t *coverage = build_glyph_mask(character, scale);
    if (coverage == NULL) {
        return NULL;
    }

    glyph_mask_cache[slot].character = character;
    glyph_mask_cache[slot].scale = scale;
    glyph_mask_cache[slot].coverage = coverage;
    return coverage;
}

void raster_destroy(void)
{
    for (size_t index = 0U; index < GLYPH_CACHE_CAPACITY; ++index) {
        free(glyph_mask_cache[index].coverage);
        glyph_mask_cache[index].coverage = NULL;
    }
}

void raster_rect(
    Canvas *buffer,
    int32_t left,
    int32_t top,
    int32_t width,
    int32_t height,
    uint32_t value)
{
    int32_t x0 = left < 0 ? 0 : left;
    int32_t x1 = left + width > buffer->width ? buffer->width : left + width;
    int32_t y0 = top < buffer->clip_top ? buffer->clip_top : top;
    int32_t y1 = top + height > buffer->clip_bottom ? buffer->clip_bottom : top + height;
    for (int32_t y = y0; y < y1; ++y)
        for (int32_t x = x0; x < x1; ++x) put_pixel(buffer, x, y, value);
}

static void draw_glyph(
    Canvas *buffer,
    char character,
    int32_t left,
    int32_t top,
    int32_t scale,
    uint32_t value)
{
    if (character == ' ') {
        return;
    }

    int32_t width = 5 * scale;
    int32_t height = 7 * scale;
    const uint8_t *coverage = cached_glyph_mask(character, scale);
    const uint8_t *rows = glyph_rows(character);

    for (int32_t y = 0; y < height; ++y) {
        for (int32_t x = 0; x < width; ++x) {
            uint8_t pixel_coverage =
                coverage != NULL
                    ? coverage[(size_t)y * (size_t)width + (size_t)x]
                    : glyph_pixel_coverage(rows, x, y, scale);
            if (pixel_coverage == 0U) {
                continue;
            }
            blend_pixel(
                buffer,
                left + x,
                top + y,
                value,
                pixel_coverage);
        }
    }
}

enum text_position {
    TEXT_NORMAL = 0,
    TEXT_SUBSCRIPT = -1,
    TEXT_SUPERSCRIPT = 1
};

struct text_token {
    char glyph;
    enum text_position position;
    size_t bytes;
};

static struct text_token next_text_token(const char *cursor)
{
    struct text_token token = {'?', TEXT_NORMAL, 1U};
    size_t prefix = 0U;
    const char *glyph = cursor;

    if ((cursor[0] == '_' || cursor[0] == '^') &&
        cursor[1] != '\0' && cursor[1] != '\n') {
        token.position = cursor[0] == '_' ? TEXT_SUBSCRIPT : TEXT_SUPERSCRIPT;
        prefix = 1U;
        glyph = cursor + 1;
    }

    if ((unsigned char)glyph[0] == 0xceU &&
        (unsigned char)glyph[1] == 0xbbU) {
        token.glyph = '\x7f';
        token.bytes = prefix + 2U;
    } else {
        token.glyph = glyph[0];
        token.bytes = prefix + 1U;
    }
    return token;
}

static int32_t positioned_scale(int32_t scale, enum text_position position)
{
    if (position == TEXT_NORMAL) return scale;
    int32_t smaller = (2 * scale + 2) / 3;
    return smaller < 1 ? 1 : smaller;
}

static int32_t positioned_advance(int32_t scale, enum text_position position)
{
    if (position == TEXT_NORMAL) return 6 * scale;
    return 4 * positioned_scale(scale,position);
}

void raster_text(
    Canvas *buffer,
    const char *text,
    int32_t left,
    int32_t top,
    int32_t scale,
    uint32_t value)
{
    int32_t x = left;
    for (const char *cursor = text; *cursor != '\0';) {
        struct text_token token = next_text_token(cursor);
        int32_t draw_scale = positioned_scale(scale,token.position);
        int32_t draw_left = x + (token.position == TEXT_NORMAL ? 0 : -scale);
        int32_t draw_top = top;
        if (token.position == TEXT_SUBSCRIPT) draw_top += 3 * scale;

        draw_glyph(buffer,token.glyph,draw_left,draw_top,draw_scale,value);
        x += positioned_advance(scale,token.position);
        cursor += token.bytes;
    }
}

int32_t raster_text_width(const char *text, int32_t scale)
{
    int32_t x = 0;
    int32_t rightmost = 0;
    for (const char *cursor = text; *cursor != '\0';) {
        struct text_token token = next_text_token(cursor);
        int32_t draw_scale = positioned_scale(scale,token.position);
        int32_t draw_left = x + (token.position == TEXT_NORMAL ? 0 : -scale);
        int32_t right = draw_left + 5 * draw_scale;
        if (right > rightmost) rightmost = right;
        x += positioned_advance(scale,token.position);
        cursor += token.bytes;
    }
    return rightmost;
}
