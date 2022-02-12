#include "PowerRange.h"
#include <sstream>

namespace goe{
    PowerRange::PowerRange(int _min, int _max): min(_min), max(_max) {}
    void PowerRange::set_min(int value){
        if(value < max) {
            min = value;
        }
    }
    int PowerRange::get_min() const { return min; }
    void PowerRange::set_max(int value){
        if(value > min) {
            max = value;
        }
    }
    int PowerRange::get_max() const { return max; }

    std::string PowerRange::to_string() const{
        std::stringstream result;
        result << "{\"min\":" << min << ", \"max\":" << max << "}";
        return result.str();
    }
}
