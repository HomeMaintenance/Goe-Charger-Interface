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
const std::vector<std::string> Charger::response_LUT{
    "Undefined", "ValueError", "HttpError", "NotNeeded", "FromDevice", "FromCache"
};
std::string Charger::response_to_str(Response r){return response_LUT[static_cast<int>(r)];}

Charger::Charger(std::string _name, std::string _address): PowerSink(_name) {
    address =  "http://" + _address;
    curl_mtx = std::make_unique<std::mutex>();
    cache = new Cache<Json::Value>();
    cache->max_age = 0;
    _online = new bool();

    GetResponse<Json::Value> device_data = get_data_from_device();
    if(device_data.response == Response::FromDevice)
        *cache = device_data.value;

    set_requesting_power(power_range_default);
}

Charger::~Charger(){
    delete cache;
    cache = nullptr;
    delete _online;
    _online = nullptr;
}

float Charger::using_power() {
    return get_nrg().value;
}

int Charger::get_min_amp() const{
    return min_amp;
}

Charger::GetResponse<int> Charger::get_amp(){
    GetResponse<Json::Value> raw_data = get_from_cache("amp", "0");
    std::string data_str = raw_data.value.asString();

    int data_int = std::stoi(data_str);

    GetResponse<int> result(data_int, raw_data.response);
    return result;
}

Charger::Response Charger::set_amp(int value){
    auto amp = get_amp();
    if(static_cast<int>(amp.response) > 0 && value == amp.value || value < min_amp || value > max_amp)
        return Response::NotNeeded;

    Response res = set_data("amp",value);
    log("amp set to " + std::to_string(get_amp().value) + ", with response " + response_to_str(res));
    return res;
}

Charger::Response Charger::set_alw(bool value){
    auto alw_res = get_alw();
    if(alw_res.response == Response::FromDevice && value == alw_res.value)
        return Response::NotNeeded;
    Response res = set_data("alw",static_cast<int>(value));
    log("alw set to " + std::to_string(get_alw().value) + ", with response " + response_to_str(res));
    return res;
}

Charger::GetResponse<int> Charger::get_nrg() const{
    auto raw_data = get_from_cache("nrg", "0");
    if(raw_data.value.type() != Json::arrayValue)
        return GetResponse<int>(0, Response::TypeError);
    int nrg = raw_data.value[11].asInt()*10; // Watts
    GetResponse<int> result(nrg, raw_data.response);
    return result;
}

Charger::GetResponse<float> Charger::get_power_factor() const{
    auto raw_data = get_from_cache("nrg", "0");
    if(raw_data.value.type() != Json::arrayValue)
        return GetResponse<float>(-1., Response::TypeError);
    const float power_factor_raw = (raw_data.value[12].asInt() + raw_data.value[13].asInt() + raw_data.value[14].asInt())/3;
    const float power_factor = static_cast<int>(power_factor_raw*100)/100;
    GetResponse<float> result(power_factor, raw_data.response);
    return result;
}

Charger::GetResponse<bool> Charger::get_alw() const{
    auto raw_data = get_from_cache("alw", "0");
    bool alw = static_cast<bool>(std::stoi(raw_data.value.asString()));
    GetResponse<bool> result(alw, raw_data.response);
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

    auto _car = get_car();
    auto _alw = get_alw();

    if(_car.response < 0){
        log("Error while reading car: " + response_to_str(_car.response));
    }

    if(_alw.response < 0){
        log("Error while reading alw: "+ response_to_str(_alw.response));
    }

    if(!_car.value){
        log("Warning: Car not connected");
    }
    if(control_mode == ControlMode::Solar){
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
            if(_alw.value || get_allowed_power() != 0){
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

Charger::GetResponse<bool> Charger::get_car() const{
    auto raw_data = get_from_cache("car", "0");
    std::string data = raw_data.value.asString();
    GetResponse<bool> result(std::stoi(data)>0, raw_data.response);
    return result;
}

Charger::Response Charger::update_cache() const {
    GetResponse<Json::Value> device_data = get_data_from_device();
    if(static_cast<int>(device_data.response) < 0)
        return device_data.response;
    cache->update(device_data.value);
    return device_data.response;
}

Charger::GetResponse<Json::Value> Charger::get_from_cache(const std::string& key, const Json::Value& defaultValue) const{
    GetResponse<Json::Value> result;
    if(cache->dirty()){
        update_cache();
        result.response = Response::FromDevice;
    }
    else{
        result.response = Response::FromCache;
    }
    auto data = (*cache->get_data()).get(key, defaultValue);
    result.value = data;
    return result;
}

void Charger::set_requesting_power(const PowerRange& range){
    PowerSink::set_requesting_power(range);
    if(control_mode != ControlMode::Solar){
        Response res = set_amp(power_to_amp(range.get_min()));
    }
}

std::size_t write_callback(const char* in, std::size_t size, std::size_t num, std::string* out);
Charger::GetResponse<Json::Value> Charger::get_data_from_device() const {
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
            return GetResponse(jsonData, Response::FromDevice);
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
    return GetResponse(Json::Value(), Response::HttpError);
}

Charger::Response Charger::set_data(std::string key, Json::Value value) const{

    if(key == "")
        return Response::TypeError;
    if(value.isObject())
        return Response::TypeError;
    if(value.isArray())
        return Response::TypeError;
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
        cache->mark_dirty();
        return Response::FromDevice;
    }
    return Response::HttpError;
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

Charger::GetResponse<Charger::AccessState> Charger::get_access_state() const{
    auto raw_data = get_from_cache("ast", "1");
    AccessState state = static_cast<AccessState>(std::stoi(raw_data.value.asString()));
    GetResponse<AccessState> result(state, raw_data.response);
    return result;
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
    result["amp"] = get_amp().value;
    result["alw"] = get_alw().value;
    Json::Value accessStateJson;
    accessStateJson["int"] = static_cast<int>(get_access_state().value);
    accessStateJson["str"] = accessStateLUT[static_cast<int>(get_access_state().value)];
    result["access_state"] = accessStateJson;
    result["online"] = online();
    result["power_factor"] = get_power_factor().value;
    result["car"] = get_car().value;
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
