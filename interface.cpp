#include "core/powerSave.h"
#include <interface.h>

namespace {
// KY-023 joystick: powered from 3V3, ADC1 inputs remain usable with Wi-Fi.
constexpr int kJoyX = JOY_X_PIN;  // GPIO34
constexpr int kJoyY = JOY_Y_PIN;  // GPIO35
constexpr int kJoyLow = 1150;
constexpr int kJoyHigh = 2950;
constexpr int kJoyLowRelease = 1550;
constexpr int kJoyHighRelease = 2550;
constexpr unsigned long kJoyRepeatDelayMs = 420;
constexpr unsigned long kJoyRepeatIntervalMs = 210;
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

StarterPackButton selectButton(kSelectButton);

class JoystickAxis {
public:
    explicit JoystickAxis(int pin) : pin_(pin) {}
    void begin() { pinMode(pin_, INPUT); }
    // Returns -1 / +1 on a new move or repeat, 0 otherwise.
    int update(unsigned long now) {
        const int reading = analogRead(pin_);
        int detected = direction_;
        if (direction_ == 0) {
            if (reading < kJoyLow) detected = -1;
            else if (reading > kJoyHigh) detected = 1;
        } else if (direction_ < 0 && reading >= kJoyLowRelease) detected = 0;
        else if (direction_ > 0 && reading <= kJoyHighRelease) detected = 0;
        if (detected != direction_) {
            direction_ = detected;
            firstPressAt_ = now;
            lastRepeatAt_ = now;
            return direction_;
        }
        if (direction_ && now - firstPressAt_ >= kJoyRepeatDelayMs &&
            now - lastRepeatAt_ >= kJoyRepeatIntervalMs) {
            lastRepeatAt_ = now;
            return direction_;
        }
        return 0;
    }
private:
    int pin_;
    int direction_ = 0;
    unsigned long firstPressAt_ = 0;
    unsigned long lastRepeatAt_ = 0;
};
JoystickAxis joystickX(kJoyX);
JoystickAxis joystickY(kJoyY);
}

void _setup_gpio() {
    joystickX.begin();
    joystickY.begin();
    analogReadResolution(12);
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

    Serial.printf("Joystick X34=%d Y35=%d SW25=%d\n", analogRead(kJoyX),
                  analogRead(kJoyY), digitalRead(kSelectButton));
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
    static bool inputTaskAnnounced = false;
    if (!inputTaskAnnounced) {
        Serial.println("StarterPack joystick input running (X34 Y35 SW25)");
        inputTaskAnnounced = true;
    }
    const unsigned long now = millis();
    const int xEvent = joystickX.update(now);
    const int yEvent = joystickY.update(now);
    selectButton.update(now);
    const bool selectDownEvent = selectButton.pressedEdge();
    const bool selectUpEvent = selectButton.releasedEdge();
    const bool joystickEvent = xEvent || yEvent;

    // Waking a sleeping screen should not trigger a menu action.
    if ((joystickEvent || selectDownEvent) && wakeUpScreen()) {
        if (selectDownEvent) suppressSelectRelease = true;
        AnyKeyPress = false;
        return;
    }
    if (selectDownEvent) {
        selectPressedAt = now;
        suppressSelectRelease = false;
    }
    const bool selectLongEvent = selectButton.pressed() && !suppressSelectRelease &&
                                 now - selectPressedAt >= kSelectLongPressMs;
    if (selectLongEvent) suppressSelectRelease = true;
    const bool selectEvent = selectUpEvent && !suppressSelectRelease;

    // Standard menus: Y up/down; X left/right. Event flags are latched until read.
    // If your particular joystick is reversed, flip the two Y/X comparisons.
    if (yEvent > 0) DownPress = true;
    if (yEvent < 0) PrevPress = true;
    if (xEvent > 0) NextPress = true;
    if (xEvent < 0) PrevPress = true;
    if (selectEvent) SelPress = true;
    if (selectLongEvent) EscPress = true;
    if (joystickEvent || selectEvent || selectLongEvent) AnyKeyPress = true;
}

void powerOff() {}

void checkReboot() {}
