#include <csignal>
#include <iostream>
#include <thread>
#include <Server.h>

bool run_servers = true;

void signal_handler(int signal){
    if(signal == SIGINT)
        run_servers = false;
}

void server_thread(int port, const std::weak_ptr<goe::Charger>& charger){
    Server server = Server(port);
    server.goeCharger = charger;
    server.run();
}

int main(int argc, char* argv[]){
    std::signal(SIGINT, signal_handler);

    std::shared_ptr<goe::Charger> goeCharger = std::make_shared<goe::Charger>("goeCharger", "192.168.178.106");

    std::thread server0(server_thread, 8800, goeCharger);

    while(run_servers){
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }

    server0.join();

    return 0;
}
