#include "OledDisplay.h"

#include <cmath>

OledDisplayClass::OledDisplayClass(int16_t width, int16_t height)
    : SerialDisplayClass(width, height), oled_(width, height, &Wire, -1) {}

uint16_t OledDisplayClass::monoColor(uint32_t color) { return color == TFT_BLACK ? SSD1306_BLACK : SSD1306_WHITE; }

void OledDisplayClass::flush() {
    if (ready_ && autoFlush_) oled_.display();
}

void OledDisplayClass::startWrite() { autoFlush_ = false; }
void OledDisplayClass::endWrite() {
    autoFlush_ = true;
    flush();
}

void OledDisplayClass::begin(int width, int height) {
    VectorDisplayClass::begin(width, height);
    Wire.begin(OLED_SDA_PIN, OLED_SCL_PIN);
    Wire.setClock(400000);
    ready_ = oled_.begin(SSD1306_SWITCHCAPVCC, OLED_ADDRESS, true, false);
    if (!ready_ && OLED_ADDRESS != 0x3D) ready_ = oled_.begin(SSD1306_SWITCHCAPVCC, 0x3D, true, false);
    if (!ready_) {
        Serial.println("OLED SSD1306 not found");
        return;
    }
    oled_.clearDisplay();
    oled_.setTextColor(SSD1306_WHITE, SSD1306_BLACK);
    oled_.setTextSize(1);
    oled_.display();
    Serial.println("OLED SSD1306 ready");
}

void OledDisplayClass::fillScreen(int32_t color) { oled_.fillScreen(monoColor(color)); flush(); }
void OledDisplayClass::drawPixel(int32_t x, int32_t y, int32_t color) { oled_.drawPixel(x, y, monoColor(color)); }
void OledDisplayClass::drawLine(int32_t x0, int32_t y0, int32_t x1, int32_t y1, int32_t color) {
    oled_.drawLine(x0, y0, x1, y1, monoColor(color)); flush();
}
void OledDisplayClass::drawFastHLine(int32_t x, int32_t y, int32_t width, int32_t color) {
    oled_.drawFastHLine(x, y, width, monoColor(color)); flush();
}
void OledDisplayClass::drawFastVLine(int32_t x, int32_t y, int32_t height, int32_t color) {
    oled_.drawFastVLine(x, y, height, monoColor(color)); flush();
}
void OledDisplayClass::drawRect(int32_t x, int32_t y, int32_t width, int32_t height, int32_t color) {
    oled_.drawRect(x, y, width, height, monoColor(color)); flush();
}
void OledDisplayClass::fillRect(int32_t x, int32_t y, int32_t width, int32_t height, int32_t color) {
    oled_.fillRect(x, y, width, height, monoColor(color)); flush();
}
void OledDisplayClass::drawRoundRect(int32_t x, int32_t y, int32_t width, int32_t height, int32_t radius, int32_t color) {
    oled_.drawRoundRect(x, y, width, height, radius, monoColor(color)); flush();
}
void OledDisplayClass::fillRoundRect(int32_t x, int32_t y, int32_t width, int32_t height, int32_t radius, int32_t color) {
    oled_.fillRoundRect(x, y, width, height, radius, monoColor(color)); flush();
}
void OledDisplayClass::drawCircle(int32_t x, int32_t y, int32_t radius, int32_t color) {
    oled_.drawCircle(x, y, radius, monoColor(color)); flush();
}
void OledDisplayClass::fillCircle(int32_t x, int32_t y, int32_t radius, int32_t color) {
    oled_.fillCircle(x, y, radius, monoColor(color)); flush();
}
void OledDisplayClass::drawTriangle(int32_t x0, int32_t y0, int32_t x1, int32_t y1, int32_t x2, int32_t y2, int32_t color) {
    oled_.drawTriangle(x0, y0, x1, y1, x2, y2, monoColor(color)); flush();
}
void OledDisplayClass::fillTriangle(int32_t x0, int32_t y0, int32_t x1, int32_t y1, int32_t x2, int32_t y2, int32_t color) {
    oled_.fillTriangle(x0, y0, x1, y1, x2, y2, monoColor(color)); flush();
}

void OledDisplayClass::drawEllipse(int16_t x, int16_t y, int32_t rx, int32_t ry, uint16_t color) {
    for (int angle = 0; angle < 360; angle += 3) {
        const float radians = angle * PI / 180.0f;
        oled_.drawPixel(x + lroundf(rx * cosf(radians)), y + lroundf(ry * sinf(radians)), monoColor(color));
    }
    flush();
}

void OledDisplayClass::fillEllipse(int16_t x, int16_t y, int32_t rx, int32_t ry, uint16_t color) {
    if (rx <= 0 || ry <= 0) return;
    for (int32_t row = -ry; row <= ry; ++row) {
        const float ratio = 1.0f - (float)(row * row) / (float)(ry * ry);
        const int32_t halfWidth = lroundf(rx * sqrtf(ratio < 0 ? 0 : ratio));
        oled_.drawFastHLine(x - halfWidth, y + row, halfWidth * 2 + 1, monoColor(color));
    }
    flush();
}

void OledDisplayClass::drawArc(int32_t x, int32_t y, int32_t radius, int32_t innerRadius,
                               uint32_t startAngle, uint32_t endAngle, uint32_t foreground,
                               uint32_t, bool) {
    if (endAngle < startAngle) std::swap(startAngle, endAngle);
    for (uint32_t angle = startAngle; angle <= endAngle; angle += 3) {
        const float radians = angle * PI / 180.0f;
        for (int32_t r = innerRadius; r <= radius; ++r) {
            oled_.drawPixel(x + lroundf(r * cosf(radians)), y + lroundf(r * sinf(radians)), monoColor(foreground));
        }
    }
    flush();
}

void OledDisplayClass::drawWideLine(float x0, float y0, float x1, float y1, float width,
                                    int32_t foreground, int32_t) {
    const int half = max(0, (int)lroundf(width / 2.0f));
    for (int offset = -half; offset <= half; ++offset)
        oled_.drawLine(lroundf(x0), lroundf(y0) + offset, lroundf(x1), lroundf(y1) + offset, monoColor(foreground));
    flush();
}

void OledDisplayClass::drawXBitmap(int16_t x, int16_t y, const uint8_t *bitmap, int16_t width,
                                   int16_t height, uint16_t color) {
    oled_.drawXBitmap(x, y, bitmap, width, height, monoColor(color)); flush();
}
void OledDisplayClass::drawXBitmap(int16_t x, int16_t y, const uint8_t *bitmap, int16_t width,
                                   int16_t height, uint16_t color, uint16_t background) {
    oled_.fillRect(x, y, width, height, monoColor(background));
    oled_.drawXBitmap(x, y, bitmap, width, height, monoColor(color));
    flush();
}

void OledDisplayClass::setCursor(int16_t x, int16_t y) {
    VectorDisplayClass::setCursor(x, y);
    oled_.setCursor(x, y);
}
int16_t OledDisplayClass::getCursorX() const { return oled_.getCursorX(); }
int16_t OledDisplayClass::getCursorY() const { return oled_.getCursorY(); }
void OledDisplayClass::setTextSize(uint8_t size) {
    // Keep TFT-oriented modules readable on the eight-row StarterPack OLED.
    size = 1;
    VectorDisplayClass::setTextSize(size);
    oled_.setTextSize(size);
}
void OledDisplayClass::setTextColor(uint16_t color) {
    VectorDisplayClass::setTextColor(color);
    oled_.setTextColor(monoColor(color));
}
void OledDisplayClass::setTextColor(uint16_t color, uint16_t background, bool) {
    VectorDisplayClass::setTextColor(color, background);
    oled_.setTextColor(monoColor(color), monoColor(background));
}
void OledDisplayClass::setTextWrap(bool wrapX, bool) {
    VectorDisplayClass::setTextWrap(wrapX);
    oled_.setTextWrap(wrapX);
}

int16_t OledDisplayClass::textWidth(const String &text, uint8_t) const { return text.length() * 6 * textsize; }
int16_t OledDisplayClass::textWidth(const char *text, uint8_t font) const { return textWidth(String(text), font); }
int16_t OledDisplayClass::fontHeight(int16_t) const { return 8 * textsize; }

int16_t OledDisplayClass::drawAlignedString(const String &text, int32_t x, int32_t y, uint8_t alignment) {
    const int16_t width = textWidth(text);
    if (alignment == TC_DATUM || alignment == MC_DATUM || alignment == BC_DATUM) x -= width / 2;
    if (alignment == TR_DATUM || alignment == MR_DATUM || alignment == BR_DATUM) x -= width;
    oled_.setCursor(x, y);
    oled_.print(text);
    cursor_x = oled_.getCursorX();
    cursor_y = oled_.getCursorY();
    flush();
    return width;
}
int16_t OledDisplayClass::drawString(const String &text, int32_t x, int32_t y, uint8_t) {
    return drawAlignedString(text, x, y, textDatum_);
}
int16_t OledDisplayClass::drawCentreString(const String &text, int32_t x, int32_t y, uint8_t) {
    return drawAlignedString(text, x, y, TC_DATUM);
}
int16_t OledDisplayClass::drawRightString(const String &text, int32_t x, int32_t y, uint8_t) {
    return drawAlignedString(text, x, y, TR_DATUM);
}

size_t OledDisplayClass::print(const String &text) {
    const size_t written = oled_.print(text);
    cursor_x = oled_.getCursorX();
    cursor_y = oled_.getCursorY();
    flush();
    return written;
}

void OledDisplayClass::invertDisplay(bool invert) { if (ready_) oled_.invertDisplay(invert); }
void OledDisplayClass::sleep(bool sleeping) {
    if (ready_) oled_.ssd1306_command(sleeping ? SSD1306_DISPLAYOFF : SSD1306_DISPLAYON);
}
uint16_t OledDisplayClass::color565(uint8_t red, uint8_t green, uint8_t blue) const {
    return ((red & 0xF8) << 8) | ((green & 0xFC) << 3) | (blue >> 3);
}
