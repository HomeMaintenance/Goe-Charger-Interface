#pragma once
#include <string>
#include <unordered_map>
#include <variant>
#include <mutex>
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
        virtual bool allow_power(float power) override;

        int get_min_amp() const;
        int get_amp();
        void set_amp(int value);
        bool get_alw() const;
        void set_alw(bool value);

        void set_control_mode(ControlMode mode);
        ControlMode get_control_mode() const;
        int power_usage() const;

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
        const int max_amp{20};
        ControlMode control_mode{ControlMode::Off};

        std::unique_ptr<std::mutex> curl_mtx;
    private:
        Json::Value get_data_from_device() const;
        bool set_data(std::string key, Json::Value value) const;
        const PowerRange power_range_default = PowerRange(min_amp * 690.f, max_amp * 690.f);
        const PowerRange power_range_off = PowerRange(0,0);
    };
}
