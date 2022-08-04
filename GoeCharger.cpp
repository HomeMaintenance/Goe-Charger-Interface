#include "GoeCharger.h"
#include <curl/curl.h>
#include <math.h>
#ifdef GOE_DEBUG
#include <iostream>
#endif



std::string prettify(const Json::Value& json){
    return json.toStyledString();
}

std::unordered_map<std::string,std::string> json_2_map(const Json::Value& json){
    std::unordered_map<std::string,std::string> result;
    for(const auto& key : json.getMemberNames()){
        if(json[key].isArray()){

        }
        else if(json[key].isObject()){

        }
        else{
            result[key] = json[key].asString();
        }
    }
    return result;
}

namespace goe{
const std::string Charger::type = "goeCharger";
Charger::Charger(std::string _name, std::string _address): PowerSink(_name) {
    address =  "http://" + _address;
    curl_mtx = std::make_unique<std::mutex>();
    cache = new Cache<Json::Value>();
    _online = new bool();
    *cache = get_data_from_device();
    set_requesting_power(power_range_default);
}

Charger::~Charger(){
    delete cache;
    cache = nullptr;
    delete _online;
    _online = nullptr;
}

float Charger::using_power() {
    return get_nrg();
}

int Charger::get_min_amp() const{
    return min_amp;
}

int Charger::get_amp(){
    auto raw_data = get_from_cache("amp", "0").asString();
    int result = std::stoi(raw_data);
    return result;
}

void Charger::set_amp(int value){
    if(value == get_amp() || value <= min_amp || value > max_amp)
        return;

    set_data("amp",value);
    log("amp set to " + std::to_string(get_amp()));
}

void Charger::set_alw(bool value){
    if(value == get_alw())
        return;
    set_data("alw",static_cast<int>(value));
    log("alw set to " + std::to_string(get_alw()));
}

int Charger::get_nrg() const{
    auto raw_data = get_from_cache("nrg", "0");
    if(raw_data.type() != Json::arrayValue)
        return 0;
    int result = raw_data[11].asInt()*10; // Watts
    return result;
}

float Charger::get_power_factor() const{
    auto raw_data = get_from_cache("nrg", "0");
    if(raw_data.type() != Json::arrayValue)
        return -1;
    float result = (raw_data[12].asInt() + raw_data[13].asInt() + raw_data[14].asInt())/3;
    return static_cast<int>(result*100)/100;
}

bool Charger::get_alw() const{
    auto raw_data = get_from_cache("alw", "0").asString();
    bool result = static_cast<bool>(std::stoi(raw_data));
    return result;
}

void Charger::set_control_mode(Charger::ControlMode mode){
    if(control_mode != mode){
        control_mode = mode;
        set_alw(control_mode == ControlMode::On);
    }
}

Charger::ControlMode Charger::get_control_mode() const{
    return control_mode;
}

bool Charger::allow_power(float power){
    bool power_used = false;
    if(!get_car()){
        log("Car not connected, ignoring power");
    }
    else if(control_mode == ControlMode::Solar){
        if(power >= get_requesting_power().get_min()){
            log("enough power, switch on");
            const int _amp = floor(power_to_amp(power));
            set_amp(_amp);
            set_alw(true);
            set_allowed_power(amp_to_power(_amp));
            power_used = true;
        }
        else{
            log("not enough power");
            if(get_alw() || get_allowed_power() != 0){
                log("switch off");
                set_amp(min_amp);
                set_alw(false);
                set_allowed_power(0);
            }
        }
    }
    else{
        log("Manual mode active, ignoring power");
    }
    return power_used;
}

bool Charger::get_car() const{
    auto raw_data = get_from_cache("car", "0").asString();
    int result = std::stoi(raw_data);
    return result;
}

bool Charger::update_cache() const {
    Json::Value device_data = get_data_from_device();
    if(device_data.empty())
        return false;
    cache->update(device_data);
    return true;
}

Json::Value Charger::get_from_cache(const std::string& key, const Json::Value& defaultValue) const{
    if(cache->dirty()){
        update_cache();
    }
    auto result = (*cache->get_data()).get(key, defaultValue);
    return result;
}

void Charger::set_requesting_power(const PowerRange& range){
    PowerSink::set_requesting_power(range);
    if(control_mode != ControlMode::Solar){
        set_amp(power_to_amp(range.get_min()));
    }
}

std::size_t write_callback(const char* in, std::size_t size, std::size_t num, std::string* out);
Json::Value Charger::get_data_from_device() const {
    const std::lock_guard<std::mutex> lock(*curl_mtx);
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
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_callback);

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
    if (httpCode == 200)
    {
        *_online = true;
        Json::Value jsonData;
        Json::Reader jsonReader;
        if (jsonReader.parse(*httpData.get(), jsonData))
        {
            #ifdef GOE_DEBUG
            std::cout << "Successfully parsed JSON data" << std::endl;
            std::cout << "\nJSON data received:" << std::endl;
            std::cout << jsonData.toStyledString() << std::endl;
            #endif
            return jsonData;
        }
        else
        {
            #ifdef GOE_DEBUG
            std::cout << "Could not parse HTTP data as JSON" << std::endl;
            std::cout << "HTTP data was:\n" << *httpData.get() << std::endl;
            #endif
        }
    }
    else{
        *_online = false;
    }
    return{};
}

bool Charger::set_data(std::string key, Json::Value value) const{

    if(key == "")
        return false;
    if(value.isObject())
        return false;
    if(value.isArray())
        return false;
    std::string request_address = address+"/mqtt?payload="+key+"="+value.asString();
    const std::lock_guard<std::mutex> lock(*curl_mtx);
    CURL* curl = curl_easy_init();
    // Set remote URL.
    curl_easy_setopt(curl, CURLOPT_URL, (request_address).c_str());
    // Don't bother trying IPv6, which would increase DNS resolution time.
    curl_easy_setopt(curl, CURLOPT_IPRESOLVE, CURL_IPRESOLVE_V4);
    // Don't wait forever, time out after 10 seconds.
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, 10);
    // Follow HTTP redirects if necessary.
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
    // Hook up data handling function.
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_callback);

    // Response information.
    long httpCode{0};
    std::unique_ptr<std::string> httpData(new std::string());

    // Hook up data container (will be passed as the last parameter to the
    // callback handling function).  Can be any pointer type, since it will
    // internally be passed as a void pointer.
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, httpData.get());

    // Run our HTTP GET command, capture the HTTP response code, and clean up.
    curl_easy_perform(curl);
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &httpCode);
    curl_easy_cleanup(curl);
    if (httpCode == 200)
    {
        Json::Value jsonData;
        Json::Reader jsonReader;
        if (jsonReader.parse(*httpData.get(), jsonData))
            cache->update(jsonData);
        else
            cache->mark_dirty();
        return true;
    }
    return false;
}

void Charger::update_device(UpdateParamData data){
    if(data.name == "ctrl"){
        set_control_mode(static_cast<Charger::ControlMode>(data.int_value));
    }
    else if(data.name == "min-amp"){
        auto power = get_requesting_power();
        power.set_min(amp_to_power(data.int_value));
        set_requesting_power(power);
        if(control_mode != ControlMode::Solar){
            set_amp(data.int_value);
        }
    }
}

float Charger::amp_to_power(float ampere){
    const float u_eff = 3*230; // Drehstrom
    const float power = ampere * u_eff;
    return power;
}

float Charger::power_to_amp(float power){
    const float u_eff = 3*230; // Drehstrom
    const float i = power/u_eff;
    return i;
}

Charger::AccessState Charger::get_access_state() const{
    auto raw_data = get_from_cache("ast", "1").asString();
    AccessState state = static_cast<AccessState>(std::stoi(raw_data));
    return state;
}

bool Charger::online() const{
    return *_online;
}

void Charger::enable_log(){
    _enable_log = true;
}

void Charger::disable_log(){
    _enable_log = false;
}

void Charger::log(std::string message) const
{
    if(_enable_log)
        std::cout << "-- GoeCharger " << name << ": " << message << std::endl;
}

Json::Value Charger::serialize(){
    Json::Value result = PowerSink::serialize();
    result["type"] = type;

    Json::Value controlModeJson;
    controlModeJson["int"] = static_cast<int>(control_mode);
    controlModeJson["str"] = controlModeLUT[static_cast<int>(control_mode)];
    result["control_mode"] = controlModeJson;
    result["amp"] = get_amp();
    result["alw"] = get_alw();
    Json::Value accessStateJson;
    accessStateJson["int"] = static_cast<int>(get_access_state());
    accessStateJson["str"] = accessStateLUT[static_cast<int>(get_access_state())];
    result["access_state"] = accessStateJson;
    result["online"] = online();
    result["power_factor"] = get_power_factor();
    result["car"] = get_car();
    return result;
}

void Charger::register_http_server_functions(httplib::Server* svr)
{
    PowerSink::register_http_server_functions(svr);
    Json::CharReaderBuilder builder;
    builder["collectComments"] = false;
    svr->Put(
        "/sink/"+name,
        [this, builder]
        (const httplib::Request &req, httplib::Response &res)
        {
            std::cout << "Put received" << std::endl;
            std::stringstream ss;
            ss.str(req.body);
            Json::Value value;
            Json::String errs;
            bool ok = Json::parseFromStream(builder, ss, &value, &errs);
            UpdateParamData data;
            data.name = value["param"].asString();
            data.str_value = value["str_value"].asString();

            auto test = std::stoi(value["int_value"].asString());
            data.int_value = test;
            update_device(data);
        }
    );
}

std::size_t write_callback(const char* in, std::size_t size, std::size_t num, std::string* out){
    const std::size_t totalBytes(size * num);
    out->append(in, totalBytes);
    return totalBytes;
}

}
