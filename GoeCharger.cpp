#include "GoeCharger.h"
#include <curl/curl.h>
#include <json/json.h>
#include <math.h>
#ifdef GOE_DEBUG
#include <iostream>
#endif

namespace goe{
Charger::Charger(std::string _name, std::string _address): name(_name) {
    address =  "http://" + _address;
    get_data_from_device();
}

Charger::~Charger(){

}

int Charger::get_error_counter() const {
    return error_counter;
}

int Charger::get_min_amp() const{
    return min_amp;
}

void Charger::set_amp(int value){
    if(value == amp)
        return;
    amp = value;
}

void Charger::set_alw(bool value)
{
    if(value == alw)
        return;
    alw = value;
}

bool Charger::get_alw() const{ return alw; }

int Charger::get_allowed_power() const{
    return allowed_power;
}

bool Charger::connected() const{
    return false;
}

void Charger::set_control_mode(Charger::ControlMode mode){
    control_mode = mode;
}

Charger::ControlMode Charger::get_control_mode() const{
    return control_mode;
}

int Charger::power_usage() const{
    return 0;
}

bool Charger::allow_power(int power){
    if(control_mode == ControlMode::Solar){
        if(power > 0){
            if(power >= requesting_power_range.get_min()){
                int _amp = floor(power_to_amp(power));
                set_amp(_amp);
                set_alw(true);
                allowed_power = amp_to_power(_amp);
            }
        }
        else{
            if(allowed_power != 0){
                set_amp(min_amp);
                set_alw(false);
                allowed_power = 0;
            }
        }
    }
    return true;
}

std::size_t callback(const char* in, std::size_t size, std::size_t num, std::string* out);
std::unordered_map<std::string, dataType> Charger::get_data_from_device() const {
    CURL* curl = curl_easy_init();
    // Set remote URL.
    curl_easy_setopt(curl, CURLOPT_URL, (address+"/status").c_str());
    // Don't bother trying IPv6, which would increase DNS resolution time.
    curl_easy_setopt(curl, CURLOPT_IPRESOLVE, CURL_IPRESOLVE_V4);
    // Don't wait forever, time out after 10 seconds.
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, 10);
    // Follow HTTP redirects if necessary.
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
    // Hook up data handling function.
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, callback);

    // Response information.
    long httpCode(0);
    std::unique_ptr<std::string> httpData(new std::string());

    // Hook up data container (will be passed as the last parameter to the
    // callback handling function).  Can be any pointer type, since it will
    // internally be passed as a void pointer.
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, httpData.get());

    // Run our HTTP GET command, capture the HTTP response code, and clean up.
    curl_easy_perform(curl);
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &httpCode);
    curl_easy_cleanup(curl);
    bool success = false;
    if (httpCode == 200)
    {
        success = true;
        #ifdef GOE_DEBUG
        Json::Value jsonData;
        Json::Reader jsonReader;
        if (jsonReader.parse(*httpData.get(), jsonData))
        {
            std::cout << "Successfully parsed JSON data" << std::endl;
            std::cout << "\nJSON data received:" << std::endl;
            std::cout << jsonData.toStyledString() << std::endl;
        }
        else
        {
            std::cout << "Could not parse HTTP data as JSON" << std::endl;
            std::cout << "HTTP data was:\n" << *httpData.get() << std::endl;
        }
        #endif
    }

    std::unordered_map<std::string, dataType> result;
    result["success"] = success;
    return result;
}

float Charger::amp_to_power(float ampere){
    float u_eff = 3*230; // Drehstrom
    float power = ampere * u_eff;
    return power;
}

float Charger::power_to_amp(float power){
    float u_eff = 3*230; // Drehstrom
    float i = power/u_eff;
    return i;
}

std::size_t callback(const char* in, std::size_t size, std::size_t num, std::string* out){
    const std::size_t totalBytes(size * num);
    out->append(in, totalBytes);
    return totalBytes;
}

}
