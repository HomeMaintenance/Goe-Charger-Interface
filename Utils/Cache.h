#pragma once
#include <ctime>
#include <vector>
#include <memory>

template<class T>
class Cache{
public:
    Cache(){
        data = std::make_shared<T>();
    };

    Cache(std::shared_ptr<T> d){
        data = d;
    };

    Cache(T d){
        data = std::make_shared<T>();
        *data = d;
    };

    ~Cache() = default;

    void update(const T* _data){
        time = std::clock();
        *data = *_data;
        dirty_flag = false;
    }

    void update(T _data){
        time = std::clock();
        *data = _data;
        dirty_flag = false;
    }

    void update(std::shared_ptr<T> _data){
        time = std::clock();
        data = _data;
        dirty_flag = false;
    }

    std::shared_ptr<T> get_data() const {
        return data;
    }

    bool dirty() const {
        std::clock_t time_now{std::clock()};
        bool result = (time + max_age) < time_now || dirty_flag;
        return result;
    }

    void mark_dirty()
    {
        dirty_flag = true;
    }

    void set_max_age(int age){
        max_age = age;
    }

    int get_max_age() const {
        return max_age;
    }

private:
    std::clock_t time;
    std::shared_ptr<T> data;
    bool dirty_flag = true;
    int max_age{3000}; //milliseconds
};
