#include "Server.h"

Json::Value convert_to_json(const std::string str){
    Json::Value jsonData;
    Json::Reader jsonReader;
    jsonReader.parse(str, jsonData);
    return jsonData;
}

std::string convert_to_string(const Json::Value& json){
    return Json::FastWriter().write(json);
}

Server::Server(int _port){
    // HTTP
    svr.Get("/hi", [this](const httplib::Request &req, httplib::Response &res) {
        Json::Value jsonData;
        jsonData["test"] = test;
        test *= 2;
        std::string content = convert_to_string(jsonData);
        res.set_content(content, "application/json");
    });

    svr.Get("/charger/alw", [this](const httplib::Request &req, httplib::Response &res) {
        Json::Value jsonData;
        if(auto c = goeCharger.lock())
            jsonData["value"] = c->get_alw();
        std::string content = convert_to_string(jsonData);
        res.set_content(content, "application/json");
    });

    svr.Get("/charger/amp", [this](const httplib::Request &req, httplib::Response &res) {
        Json::Value jsonData;
        if(auto c = goeCharger.lock())
            jsonData["value"] = c->get_amp();
        std::string content = convert_to_string(jsonData);
        res.set_content(content, "application/json");
    });

    svr.Post("/charger/alw", [this](const httplib::Request &req, httplib::Response &res) {
        Json::Value jsonData = convert_to_json(req.body);
        if(jsonData.isMember("value")){
            bool value = jsonData["value"].asBool();
            std::cout << "value: " << value << std::endl;
            if(auto c = goeCharger.lock()){
                c->set_alw(value);
                res.status = 200;
            }
            else
                res.status = 500;
        }
        else{
            res.status = 400;
        }
    });

    svr.Post("/charger/amp", [this](const httplib::Request &req, httplib::Response &res) {
        Json::Value jsonData = convert_to_json(req.body);
        if(jsonData.isMember("value")){
            int value = jsonData["value"].asInt();
            std::cout << "value: "<< value << std::endl;
            if(auto c = goeCharger.lock()){
                c->set_amp(value);
                res.status = 200;
            }
            else
                res.status = 500;
        }
        else{
            res.status = 400;
        }
    });

    svr.set_error_handler([](const httplib::Request &req, httplib::Response &res) {
        const char *fmt = "<p>Error Status: <span style='color:red;'>%d</span></p>";
        char buf[BUFSIZ];
        snprintf(buf, sizeof(buf), fmt, res.status);
        res.set_content(buf, "text/html");
    });

    port = _port;
}

void Server::run(){
    svr.listen("0.0.0.0", port);
}

void Server::stop(){
    svr.stop();
}
