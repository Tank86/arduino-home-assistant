#include "HALight.h"
#ifdef ARDUINOHA_LIGHT

#include "../HAMqtt.h"
#include "../HADevice.h"

const char* HALight::PercentageCommandTopic = {"sct"};  	//TODO rename to brightness topic
const char* HALight::PercentageStateTopic = {"sst"};	//TODO rename to brightness topic

HALight::HALight(const char* uniqueId) :
    BaseDeviceType("light", uniqueId),
    _features(features),
    _currentState(false),
    _stateCallback(nullptr),
    _currentBrightness(0),
    _brightnessCallback(nullptr),
    _retain(false),
    _icon(nullptr)
{

}

HALight::HALight(const char* uniqueId, HAMqtt& mqtt) :
    HALight(uniqueId)
{
    (void)mqtt;
}

void HALight::onMqttConnected()
{
    if (strlen(uniqueId()) == 0) {
        return;
    }

    publishConfig();
    publishAvailability();

    DeviceTypeSerializer::mqttSubscribeTopic(
        this,
        DeviceTypeSerializer::CommandTopic
    );

    DeviceTypeSerializer::mqttSubscribeTopic(
        this,
        PercentageCommandTopic
    );

    if (!_retain) {
        publishState(_currentState);
        publishBrightness(_currentBrightness);
    }
}

void HALight::onMqttMessage(
    const char* topic,
    const uint8_t* payload,
    const uint16_t& length
)
{
    (void)payload;

    if (compareTopics(topic, DeviceTypeSerializer::CommandTopic)) {
        bool state = (length == strlen(DeviceTypeSerializer::StateOn));
        setState(state, true);
    } else if (compareTopics(topic, PercentageCommandTopic)) {
        char brightnessStr[length + 1];
        memset(brightnessStr, 0, sizeof(brightnessStr));
        memcpy(brightnessStr, payload, length);
        int32_t brightness = atoi(brightnessStr);
        if (brightness >= 0) {
            setBrightness(brightness);
        }
    }
}

bool HALight::setState(bool state, bool force)
{
    if (!force && _currentState == state) {
        return true;
    }

    if (publishState(state)) {
        _currentState = state;

        if (_stateCallback) {
            _stateCallback(state);
        }

        return true;
    }

    return false;
}

bool HALight::setBrightness(uint8_t brightness)
{
    if (publishBrightness(brightness)) {
        _currentBrightness = brightness;

        if (_brightnessCallback) {
            _brightnessCallback(_currentBrightness);
        }

        return true;
    }

    return false;
}

bool HALight::publishState(bool state)
{
    if (strlen(uniqueId()) == 0) {
        return false;
    }

    return DeviceTypeSerializer::mqttPublishMessage(
        this,
        DeviceTypeSerializer::StateTopic,
        (
            state ?
            DeviceTypeSerializer::StateOn :
            DeviceTypeSerializer::StateOff
        )
    );
}

bool HALight::publishBrightness(uint8_t brightness);
{
    if (strlen(uniqueId()) == 0) {
        return false;
    }

    uint8_t digitsNb = floor(log10(brightness)) + 1;
    char str[digitsNb + 1]; // + null terminator
    memset(str, 0, sizeof(str));
    itoa(brightness, str, 10);

    return DeviceTypeSerializer::mqttPublishMessage(
        this,
        PercentageStateTopic,
        str
    );
}

uint16_t HALight::calculateSerializedLength(const char* serializedDevice) const
{
    if (serializedDevice == nullptr) {
        return 0;
    }

    uint16_t size = 0;
    size += DeviceTypeSerializer::calculateBaseJsonDataSize();
    size += DeviceTypeSerializer::calculateNameFieldSize(getName());
    size += DeviceTypeSerializer::calculateUniqueIdFieldSize(uniqueId());
    size += DeviceTypeSerializer::calculateDeviceFieldSize(serializedDevice);
    size += DeviceTypeSerializer::calculateAvailabilityFieldSize(this);
    size += DeviceTypeSerializer::calculateRetainFieldSize(_retain);

    // command topic
    {
        const uint16_t& topicLength = DeviceTypeSerializer::calculateTopicLength(
            componentName(),
            uniqueId(),
            DeviceTypeSerializer::CommandTopic,
            false
        );

        if (topicLength == 0) {
            return 0;
        }

        // Field format: "cmd_t":"[TOPIC]"
        size += topicLength + 10; // 10 - length of the JSON decorators for this field
    }

    // state topic
    {
        const uint16_t& topicLength = DeviceTypeSerializer::calculateTopicLength(
            componentName(),
            uniqueId(),
            DeviceTypeSerializer::StateTopic,
            false
        );

        if (topicLength == 0) {
            return 0;
        }

        // Field format: ,"stat_t":"[TOPIC]"
        size += topicLength + 12; // 12 - length of the JSON decorators for this field
    }

    // icon
    {
        // icon
        if (_icon != nullptr) {
            // Field format: ,"ic":"[ICON]"
            size += strlen(_icon) + 8; // 8 - length of the JSON decorators for this field
        }

    }

    // brightness
    {
        // percentage command topic
        {
            const uint16_t& topicLength = DeviceTypeSerializer::calculateTopicLength(
                componentName(),
                uniqueId(),
                PercentageCommandTopic,
                false
            );

            if (topicLength == 0) {
                return 0;
            }

            // Field format: ,"pct_cmd_t":"[TOPIC]"
            size += topicLength + 15; // 15 - length of the JSON decorators for this field
        }

        // percentage state topic
        {
            const uint16_t& topicLength = DeviceTypeSerializer::calculateTopicLength(
                componentName(),
                uniqueId(),
                PercentageStateTopic,
                false
            );

            if (topicLength == 0) {
                return 0;
            }

            // Field format: ,"pct_stat_t":"[TOPIC]"
            size += topicLength + 16; // 16 - length of the JSON decorators for this field
        }

#if TODO_BRIGHTNESSRANGE
        // brightness range min
        if (_speedRangeMin != 1) {
            uint8_t digitsNb = floor(log10(_speedRangeMin)) + 1;

            // Field format: ,"spd_rng_min":[VALUE]
            size += digitsNb + 15; // 15 - length of the JSON decorators for this field
        }

        // brightness range max
        if (_speedRangeMax != 100) {
            uint8_t digitsNb = floor(log10(_speedRangeMax)) + 1;

            // Field format: ,"spd_rng_max":[VALUE]
            size += digitsNb + 15; // 15 - length of the JSON decorators for this field
        }
#endif
    }

    return size; // excludes null terminator
}

bool HALight::writeSerializedData(const char* serializedDevice) const
{
    if (serializedDevice == nullptr) {
        return false;
    }

    DeviceTypeSerializer::mqttWriteBeginningJson();

    // command topic
    {
        static const char Prefix[] PROGMEM = {"\"cmd_t\":\""};
        DeviceTypeSerializer::mqttWriteTopicField(
            this,
            Prefix,
            DeviceTypeSerializer::CommandTopic
        );
    }

    // state topic
    {
        static const char Prefix[] PROGMEM = {",\"stat_t\":\""};
        DeviceTypeSerializer::mqttWriteTopicField(
            this,
            Prefix,
            DeviceTypeSerializer::StateTopic
        );
    }

    // icon
    if (_icon != nullptr) {
        static const char Prefix[] PROGMEM = {",\"ic\":\""};
        DeviceTypeSerializer::mqttWriteConstCharField(
            Prefix,
            _icon
        );
    }

    // speeds
    {
        // percentage command topic
        {
            static const char Prefix[] PROGMEM = {",\"pct_cmd_t\":\""};
            DeviceTypeSerializer::mqttWriteTopicField(
                this,
                Prefix,
                PercentageCommandTopic
            );
        }

        // percentage state topic
        {
            static const char Prefix[] PROGMEM = {",\"pct_stat_t\":\""};
            DeviceTypeSerializer::mqttWriteTopicField(
                this,
                Prefix,
                PercentageStateTopic
            );
        }

#if TODO_BRIGHTNESSRANGE
        // brightness range min
        if (_speedRangeMin != 1) {
            uint8_t digitsNb = floor(log10(_speedRangeMin)) + 1;
            char str[digitsNb + 1]; // + null terminator
            memset(str, 0, sizeof(str));
            itoa(_speedRangeMin, str, 10);

            static const char Prefix[] PROGMEM = {",\"spd_rng_min\":"};
            DeviceTypeSerializer::mqttWriteConstCharField(
                Prefix,
                str,
                false
            );
        }

        // brightness range max
        if (_speedRangeMax != 100) {
            uint8_t digitsNb = floor(log10(_speedRangeMax)) + 1;
            char str[digitsNb + 1]; // + null terminator
            memset(str, 0, sizeof(str));
            itoa(_speedRangeMax, str, 10);

            static const char Prefix[] PROGMEM = {",\"spd_rng_max\":"};
            DeviceTypeSerializer::mqttWriteConstCharField(
                Prefix,
                str,
                false
            );
        }
#endif
    }

    DeviceTypeSerializer::mqttWriteNameField(getName());
    DeviceTypeSerializer::mqttWriteUniqueIdField(uniqueId());
    DeviceTypeSerializer::mqttWriteDeviceField(serializedDevice);
    DeviceTypeSerializer::mqttWriteAvailabilityField(this);
    DeviceTypeSerializer::mqttWriteRetainField(_retain);
    DeviceTypeSerializer::mqttWriteEndJson();

    return true;
}

#endif