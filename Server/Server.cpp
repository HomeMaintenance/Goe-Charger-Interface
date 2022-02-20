#include "Server.h"

Json::Value convert_to_json(const std::string str){
    Json::Value jsonData;
    Json::Reader jsonReader;
    jsonReader.parse(str, jsonData);
    return jsonData;
}

Server::Server(int _port){
    // HTTP
    svr.Get("/hi", [this](const httplib::Request &req, httplib::Response &res) {
        std::string content = "{\"test\":"+std::to_string(test)+"}";
        test *= 2;
        res.set_content(content, "application/json");
    });

    svr.Get("/charger/alw", [this](const httplib::Request &req, httplib::Response &res) {
        std::string content = "{\"value\":"+std::to_string(goeCharger.lock()->get_alw())+"}";
        res.set_content(content, "application/json");
    });

    svr.Get("/charger/amp", [this](const httplib::Request &req, httplib::Response &res) {
        std::string content = "{\"value\":"+std::to_string(goeCharger.lock()->get_amp())+"}";
        res.set_content(content, "application/json");
    });

    svr.Post("/charger/amp", [this](const httplib::Request &req, httplib::Response &res) {
        Json::Value jsonData = convert_to_json(req.body);
        if(jsonData.isMember("value")){
            int value = jsonData["value"].asInt();
            std::cout << "value: "+ value << std::endl;
            goeCharger.lock()->set_amp(value);
            res.status = 200;
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
    std::cout << "listening on port " << port << std::endl;
}
