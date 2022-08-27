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
        static const std::string type;

        std::string address;
        std::unordered_map<std::string, dataType> data;

        enum ControlMode{
            Off,
            On,
            Solar
        };

        enum AccessState{
            OPEN,
            RFID_APP_REQ,
            PRICE_AUTO
        };

    public:

        virtual float using_power() override;
        virtual bool allow_power(float power) override;

        bool get_car() const;
        int get_min_amp() const;
        int get_amp();
        void set_amp(int value);
        bool get_alw() const;
        void set_alw(bool value);

        int get_nrg() const;
        float get_power_factor() const;

        void set_control_mode(ControlMode mode);
        ControlMode get_control_mode() const;
        int power_usage() const;

        static float amp_to_power(float ampere);
        static float power_to_amp(float power);

        AccessState get_access_state() const;

        bool online() const;

        void enable_log();
        void disable_log();

        virtual Json::Value serialize() override;

        virtual void register_http_server_functions(httplib::Server* svr) override;

        void set_cache_max_age(int age);
        int get_cache_max_age() const;

    protected:
    private:
        Cache<Json::Value>* cache;
        bool update_cache() const;
        Json::Value get_from_cache(const std::string& key, const Json::Value& defaultValue) const;

        virtual void set_requesting_power(const PowerRange& range) override;

        bool alw{false};
        int amp{0};
        const int min_amp{6};
        const int max_amp{20};
        ControlMode control_mode{ControlMode::Off};
        std::vector<std::string> controlModeLUT{"Off","On","Solar"};
        AccessState accessState{AccessState::RFID_APP_REQ};

        const std::vector<std::string> accessStateLUT{"Open", "RFID_APP_REQ", "PRICE_AUTO", "RFID_APP_REQ & PRICE_AUTO"};

        bool _enable_log{false};
        void log(std::string message) const;

        bool* _online;

        std::unique_ptr<std::mutex> curl_mtx;
    private:
        Json::Value get_data_from_device() const;
        bool set_data(std::string key, Json::Value value) const;
        const PowerRange power_range_default = PowerRange(min_amp * 690.f, max_amp * 690.f);
        const PowerRange power_range_off = PowerRange(0,0);


        struct UpdateParamData{
            std::string name;
            std::string str_value;
            int int_value;
        };

        void update_device(UpdateParamData data);
    };
}
