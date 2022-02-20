#pragma once
#include <httplib.h>
#include <GoeCharger.h>

class Server{
public:
    Server(int port = 8085);

    void run();

    std::weak_ptr<goe::Charger> goeCharger;

private:
    httplib::Server svr;
    int port;
    float test = 0.23f;
};
