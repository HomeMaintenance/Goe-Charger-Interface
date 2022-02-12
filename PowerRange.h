#pragma once
#include <string>

namespace goe{
    class PowerRange{
    public:
        PowerRange(int min, int max);
        void set_min(int value);
        int get_min() const;
        void set_max(int value);
        int get_max() const;
        std::string to_string() const;
    private:
        int min;
        int max;
    };
}
