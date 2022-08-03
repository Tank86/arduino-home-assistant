#ifndef AHA_LIGHT_H
#define AHA_LIGHT_H

#include "BaseDeviceType.h"

#ifdef ARDUINOHA_LIGHT

#define HALIGHT_STATE_CALLBACK_BOOL(name) void (*name)(bool)
#define HALIGHT_STATE_CALLBACK_BRIGHTNESS(name) void (*name)(uint8_t)
#define HALIGHT_STATE_CALLBACK_RGB(name) void (*name)(uint8_t, uint8_t, uint8_t)

class HALight : public BaseDeviceType
{
public:
    static const char* BrightnessCommandTopic;
    static const char* BrightnessStateTopic;
    static const char* RGBCommandTopic;
    static const char* RGBStateTopic;

    HALight(const char* uniqueId);
    HALight(const char* uniqueId, HAMqtt& mqtt); // legacy constructor

    virtual void onMqttConnected() override;
    virtual void onMqttMessage(
        const char* topic,
        const uint8_t* payload,
        const uint16_t& length
    ) override;

    /**
     * Changes state of the light and publishes MQTT message.
     * Please note that if a new value is the same as previous one,
     * the MQTT message won't be published.
     *
     * @param state New state of the light (on - true, off - false).
     * @param force Forces to update state without comparing it to previous known state.
     * @returns Returns true if MQTT message has been published successfully.
     */
    bool setState(bool state, bool force = false);

    /**
     * Alias for setState(true).
     */
    inline bool turnOn()
        { return setState(true); }

    /**
     * Alias for setState(false).
     */
    inline bool turnOff()
        { return setState(false); }

    /**
     * Returns last known state of the light.
     * If setState method wasn't called the initial value will be returned.
     */
    inline bool getState() const
        { return _currentState; }

    /**
     * Registers callback that will be called each time the state of the light changes.
     * Please note that it's not possible to register multiple callbacks for the same light.
     *
     * @param callback
     */
    inline void onStateChanged(HALIGHT_STATE_CALLBACK_BOOL(callback))
        { _stateCallback = callback; }

    /**
     * Sets brightness of the light.
     *
     * @param brightness
     */
    bool setBrightness(uint8_t brightness);

    /**
     * Returns current brightness of the light.
     */
    inline uint8_t getBrightness() const
        { return _currentBrightness; }
		
    /**
     * Registers callback that will be called each time the state of the light changes.
     * Please note that it's not possible to register multiple callbacks for the same light.
     *
     * @param callback
     */
    inline void onBrightnessChanged(HALIGHT_STATE_CALLBACK_BRIGHTNESS(callback))
        { _brightnessCallback = callback; }


    /**
     * Sets light color.
     *
     * @param red
     * @param green
     * @param blue
     */
    bool setColor(uint8_t red, uint8_t green, uint8_t blue);

    /**
     * Returns current color of the light.
     */
    inline uint8_t getRed() const
        { return _colorRed; }
		
    /**
     * Returns current color of the light.
     */
    inline uint8_t getGreen() const
        { return _colorGreen; }

    /**
     * Returns current color of the light.
     */
    inline uint8_t getBlue() const
        { return _colorBlue; }

    /**
     * Registers callback that will be called each time the state of the light changes.
     * Please note that it's not possible to register multiple callbacks for the same light.
     *
     * @param callback
     */
    inline void onColorChanged(HALIGHT_STATE_CALLBACK_RGB(callback))
        { _colorCallback = callback; }


    /**
     * Sets `retain` flag for commands published by Home Assistant.
     * By default it's set to false.
     *
     * @param retain
     */
    inline void setRetain(bool retain)
        { _retain = retain; }


    /**
     * Sets icon of the sensor, e.g. `mdi:home`.
     *
     * @param icon Material Design Icon name with mdi: prefix.
     */
    inline void setIcon(const char* icon)
        { _icon = icon; }

private:
    bool publishState(bool state);
    bool publishBrightness(uint8_t brightness);
    bool publishRGBColor(uint8_t red, uint8_t green, uint8_t blue);
    uint16_t calculateSerializedLength(const char* serializedDevice) const override;
    bool writeSerializedData(const char* serializedDevice) const override;

    //TODO check if needed //const uint8_t _features;
    bool _currentState;
    HALIGHT_STATE_CALLBACK_BOOL(_stateCallback);
    uint16_t _currentBrightness;
    HALIGHT_STATE_CALLBACK_BRIGHTNESS(_brightnessCallback);
    uint8_t _colorRed;
    uint8_t _colorGreen;
    uint8_t _colorBlue;
    HALIGHT_STATE_CALLBACK_RGB(_colorCallback);

    bool _retain;
    const char* _icon;
};

#endif
#endif
