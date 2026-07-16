#pragma once

#include <Adafruit_SSD1306.h>
#include <VectorDisplay.h>
#include <Wire.h>

class OledDisplayClass : public SerialDisplayClass {
public:
    explicit OledDisplayClass(int16_t width = TFT_WIDTH, int16_t height = TFT_HEIGHT);

    void begin(int width = TFT_WIDTH, int height = TFT_HEIGHT) override;
    void init(uint8_t = 0) { begin(TFT_WIDTH, TFT_HEIGHT); }
    void startWrite();
    void endWrite();

    void fillScreen(int32_t color);
    void drawPixel(int32_t x, int32_t y, int32_t color);
    void drawLine(int32_t x0, int32_t y0, int32_t x1, int32_t y1, int32_t color);
    void drawFastHLine(int32_t x, int32_t y, int32_t width, int32_t color);
    void drawFastVLine(int32_t x, int32_t y, int32_t height, int32_t color);
    void drawRect(int32_t x, int32_t y, int32_t width, int32_t height, int32_t color);
    void fillRect(int32_t x, int32_t y, int32_t width, int32_t height, int32_t color);
    void drawRoundRect(int32_t x, int32_t y, int32_t width, int32_t height, int32_t radius, int32_t color);
    void fillRoundRect(int32_t x, int32_t y, int32_t width, int32_t height, int32_t radius, int32_t color);
    void drawCircle(int32_t x, int32_t y, int32_t radius, int32_t color);
    void fillCircle(int32_t x, int32_t y, int32_t radius, int32_t color);
    void drawTriangle(int32_t x0, int32_t y0, int32_t x1, int32_t y1, int32_t x2, int32_t y2, int32_t color);
    void fillTriangle(int32_t x0, int32_t y0, int32_t x1, int32_t y1, int32_t x2, int32_t y2, int32_t color);
    void drawEllipse(int16_t x, int16_t y, int32_t rx, int32_t ry, uint16_t color);
    void fillEllipse(int16_t x, int16_t y, int32_t rx, int32_t ry, uint16_t color);
    void drawArc(int32_t x, int32_t y, int32_t radius, int32_t innerRadius, uint32_t startAngle,
                 uint32_t endAngle, uint32_t foreground, uint32_t background, bool smooth = true);
    void drawWideLine(float x0, float y0, float x1, float y1, float width, int32_t foreground,
                      int32_t background = 0x00FFFFFF);
    void drawXBitmap(int16_t x, int16_t y, const uint8_t *bitmap, int16_t width, int16_t height,
                     uint16_t color);
    void drawXBitmap(int16_t x, int16_t y, const uint8_t *bitmap, int16_t width, int16_t height,
                     uint16_t color, uint16_t background);

    void setCursor(int16_t x, int16_t y);
    int16_t getCursorX() const;
    int16_t getCursorY() const;
    void setTextSize(uint8_t size);
    void setTextColor(uint16_t color);
    void setTextColor(uint16_t color, uint16_t background, bool = false);
    void setTextWrap(bool wrapX, bool wrapY = false);
    void setTextDatum(uint8_t datum) { textDatum_ = datum; }
    uint8_t getTextDatum() const { return textDatum_; }
    void setTextFont(uint8_t) {}

    int16_t drawString(const String &text, int32_t x, int32_t y, uint8_t font = 1);
    int16_t drawCentreString(const String &text, int32_t x, int32_t y, uint8_t font = 1);
    int16_t drawRightString(const String &text, int32_t x, int32_t y, uint8_t font = 1);
    int16_t textWidth(const String &text, uint8_t font = 1) const;
    int16_t textWidth(const char *text, uint8_t font = 1) const;
    int16_t fontHeight(int16_t font = 1) const;

    size_t print(const String &text);
    void invertDisplay(bool invert);
    void sleep(bool sleeping);
    uint16_t color565(uint8_t red, uint8_t green, uint8_t blue) const;

    bool ready() const { return ready_; }

private:
    static uint16_t monoColor(uint32_t color);
    void flush();
    int16_t drawAlignedString(const String &text, int32_t x, int32_t y, uint8_t alignment);

    Adafruit_SSD1306 oled_;
    bool ready_ = false;
    bool autoFlush_ = true;
    uint8_t textDatum_ = TL_DATUM;
};
