#pragma once
#include <string>
#include <unordered_map>
#include <variant>
#include "PowerRange.h"



namespace goe{
    using dataType = std::variant<std::string, int, float, bool>;

    class Charger{
    public:
        Charger(std::string name, std::string address);
        ~Charger();
        std::string name;
        std::string address;
        std::unordered_map<std::string, dataType> data;

        enum ControlMode{
            Off,
            On,
            Solar
        };

    public:
        int get_error_counter() const;

        int get_min_amp() const;
        void set_amp(int value);
        void set_alw(bool value);
        bool get_alw() const;

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
        bool alw{false};
        int amp{0};
        const int min_amp{6};
        int error_counter{0};
        PowerRange requesting_power_range{PowerRange(6,20)};
        int allowed_power{0};
        ControlMode control_mode{ControlMode::Off};

    private:
        std::unordered_map<std::string, dataType> get_data_from_device() const;
    };
}
