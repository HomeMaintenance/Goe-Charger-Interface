#pragma once
#include <string>
#include <unordered_map>
#include <variant>
#include <json/json.h>
#include <PowerSink.h>
#include <MqttClient.h>
#include "Utils/Cache.h"



namespace goe{
    using dataType = std::variant<std::string, int, float, bool>;

    class Charger: public PowerSink{
    public:
        Charger(std::string name, std::string address);
        ~Charger();
        std::string address;
        std::unordered_map<std::string, dataType> data;

        enum ControlMode{
            Off,
            On,
            Solar
        };

    public:

        virtual float using_power() override;

        int get_error_counter() const;

        int get_min_amp() const;
        void set_amp(int value);
        bool get_alw() const;
        void set_alw(bool value);

        int get_allowed_power() const;
        bool connected() const;


        void set_control_mode(ControlMode mode);
        ControlMode get_control_mode() const;
        int power_usage() const;
        bool allow_power(int power);

        static float amp_to_power(float ampere);
        static float power_to_amp(float power);

    protected:
    private:
        Cache<Json::Value>* cache;
        bool update_cache() const;
        Json::Value get_from_cache(const std::string& key, const Json::Value& defaultValue) const;

        bool alw{false};
        int amp{0};
        const int min_amp{6};
        int error_counter{0};
        PowerRange requesting_power_range{PowerRange(6,20)};
        int allowed_power{0};
        ControlMode control_mode{ControlMode::Off};

    private:
        Json::Value get_data_from_device() const;
        bool set_data(std::string key, Json::Value value) const;
    };
}
