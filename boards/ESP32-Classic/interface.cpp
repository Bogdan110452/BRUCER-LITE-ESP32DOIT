#include "core/powerSave.h"
#include <interface.h>

namespace {
constexpr int kDownButton = DW_BTN;     // D32: down / next option
constexpr int kRightButton = RIGHT_BTN; // D33: up in lists / right when contextual
constexpr int kSelectButton = SEL_BTN;  // D25: confirm; hold to go back
constexpr unsigned long kSelectLongPressMs = 700;
constexpr unsigned long kEmergencyRestartMs = 2000;
constexpr int kIrTxPin = 2;
constexpr int kIrRxPin = 35;
constexpr int kRfTxPin = 17;
constexpr int kRfRxPin = 34;
constexpr gpio_num_t kUartRxPin = GPIO_NUM_16;
constexpr gpio_num_t kUartTxPin = GPIO_NUM_17;

constexpr unsigned long kButtonDebounceMs = 20;

class StarterPackButton {
public:
    explicit StarterPackButton(int pin) : pin_(pin) {}

    void begin() {
        // The StarterPack buttons close the GPIO to GND.  Keep one stable
        // pull-up configuration: changing the pull direction while sampling
        // made some button modules look permanently open.
        pinMode(pin_, INPUT_PULLUP);
        rawPressed_ = digitalRead(pin_) == LOW;
        stablePressed_ = rawPressed_;
        rawChangedAt_ = millis();
    }

    void update(unsigned long now) {
        pressedEdge_ = false;
        releasedEdge_ = false;

        const bool electricalPressed = digitalRead(pin_) == LOW;
        if (electricalPressed != rawPressed_) {
            rawPressed_ = electricalPressed;
            rawChangedAt_ = now;
        }

        if (stablePressed_ != rawPressed_ && now - rawChangedAt_ >= kButtonDebounceMs) {
            stablePressed_ = rawPressed_;
            pressedEdge_ = stablePressed_;
            releasedEdge_ = !stablePressed_;
        }
    }

    bool pressed() const { return stablePressed_; }
    bool pressedEdge() const { return pressedEdge_; }
    bool releasedEdge() const { return releasedEdge_; }

private:
    const int pin_;
    bool rawPressed_ = false;
    bool stablePressed_ = false;
    bool pressedEdge_ = false;
    bool releasedEdge_ = false;
    unsigned long rawChangedAt_ = 0;
};

StarterPackButton downButton(kDownButton);
StarterPackButton rightButton(kRightButton);
StarterPackButton selectButton(kSelectButton);
}

void _setup_gpio() {
    downButton.begin();
    rightButton.begin();
    selectButton.begin();

    // GPIO 21/22 are dedicated to the OLED I2C bus. Keep the generic
    // IR, RF, UART and GPS defaults away from the display bus so opening
    // one of those tools cannot reconfigure the OLED pins.
    bruceConfigPins.irTx = kIrTxPin;
    bruceConfigPins.irRx = kIrRxPin;
    bruceConfigPins.rfTx = kRfTxPin;
    bruceConfigPins.rfRx = kRfRxPin;
    bruceConfigPins.uart_bus = BruceConfigPins::UARTPins(kUartRxPin, kUartTxPin);
    bruceConfigPins.gps_bus = BruceConfigPins::UARTPins(kUartRxPin, kUartTxPin);

    Serial.printf(
        "StarterPack buttons: D32=%d D33=%d D25=%d (pressed=LOW)\n",
        digitalRead(kDownButton),
        digitalRead(kRightButton),
        digitalRead(kSelectButton)
    );
}

void _post_setup_gpio() {
    // Always enter the local menu.  WebUI remains available from the Files
    // menu, but must not draw its large TFT-oriented status screen at boot.
    bruceConfig.startupApp = "";
}

int getBattery() { return 0; }

bool isCharging() { return false; }

void _setBrightness(uint8_t) {}

void InputHandler(void) {
    static bool suppressSelectRelease = false;
    static unsigned long selectPressedAt = 0;
    static bool emergencyRestartActive = false;
    static unsigned long emergencyRestartStartedAt = 0;
    static bool inputTaskAnnounced = false;
    static int lastRawDown = HIGH;
    static int lastRawRight = HIGH;
    static int lastRawSelect = HIGH;

    if (!inputTaskAnnounced) {
        Serial.println("StarterPack input task running");
        inputTaskAnnounced = true;
    }

    // Log raw electrical transitions before debounce.  These messages make
    // a wiring/pin issue distinguishable from a menu event issue.
    const int rawDown = digitalRead(kDownButton);
    const int rawRight = digitalRead(kRightButton);
    const int rawSelect = digitalRead(kSelectButton);
    if (rawDown != lastRawDown || rawRight != lastRawRight || rawSelect != lastRawSelect) {
        Serial.printf("BTN RAW D32=%d D33=%d D25=%d\n", rawDown, rawRight, rawSelect);
        lastRawDown = rawDown;
        lastRawRight = rawRight;
        lastRawSelect = rawSelect;
    }

    const unsigned long now = millis();
    downButton.update(now);
    rightButton.update(now);
    selectButton.update(now);

    const bool downPressed = downButton.pressed();
    const bool rightPressed = rightButton.pressed();
    const bool selectPressed = selectButton.pressed();
    const bool allPressed = downPressed && rightPressed && selectPressed;

    // Emergency recovery that works without a computer: hold all three
    // physical buttons for two seconds. Suppress their individual actions
    // while the combination is active so it cannot alter the current screen.
    if (allPressed) {
        if (!emergencyRestartActive) {
            emergencyRestartActive = true;
            emergencyRestartStartedAt = now;
            suppressSelectRelease = true;
        }

        SelPress = false;
        EscPress = false;
        PrevPress = false;
        DownPress = false;
        NextPress = false;
        AnyKeyPress = true;

        if (now - emergencyRestartStartedAt >= kEmergencyRestartMs) {
            Serial.println("Emergency three-button restart");
            Serial.flush();
            ESP.restart();
        }
        return;
    }
    emergencyRestartActive = false;

    const bool downEvent = downButton.pressedEdge();
    const bool rightEvent = rightButton.pressedEdge();
    const bool selectDownEvent = selectButton.pressedEdge();
    const bool selectUpEvent = selectButton.releasedEdge();

    // The first press only wakes a sleeping display; it must not also move
    // or activate an item.
    if ((downEvent || rightEvent || selectDownEvent) && wakeUpScreen()) {
        if (selectDownEvent) suppressSelectRelease = true;
        AnyKeyPress = false;
        return;
    }

    if (selectDownEvent) {
        selectPressedAt = now;
        suppressSelectRelease = false;
    }

    const bool selectLongEvent =
        selectPressed && !suppressSelectRelease && now - selectPressedAt >= kSelectLongPressMs;
    if (selectLongEvent) suppressSelectRelease = true;

    // Select fires only after a debounced release. A long press permanently
    // suppresses that release, including any mechanical bounce that follows.
    const bool selectEvent = selectUpEvent && !suppressSelectRelease;

    // Latch every edge until the UI consumes it through check().  Do not
    // overwrite an unconsumed event on the following 10 ms polling cycle.
    // The edge detectors still guarantee one action per physical press.
    if (downEvent) {
        DownPress = true;
        Serial.println("BTN D32: DOWN");
    }
    if (rightEvent) {
        NextPress = true;
        Serial.println("BTN D33: RIGHT");
    }
    if (selectEvent) {
        SelPress = true;
        Serial.println("BTN D25: SELECT");
    }
    if (selectLongEvent) {
        EscPress = true;
        Serial.println("BTN D25: BACK");
    }
    if (downEvent || rightEvent || selectEvent || selectLongEvent) AnyKeyPress = true;
}

void powerOff() {}

void checkReboot() {}
