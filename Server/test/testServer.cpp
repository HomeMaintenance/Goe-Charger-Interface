#include <csignal>
#include <iostream>
#include <thread>
#include <Server.h>

bool run_servers = true;

void signal_handler(int signal){
    std::cout << signal << std::endl;
    if(signal == SIGINT)
        run_servers = false;
}

void server_thread_func(std::weak_ptr<Server> server){
    server.lock()->run();
}

int main(int argc, char* argv[]){
    std::signal(SIGINT, signal_handler);

    std::shared_ptr<Server> server = std::make_shared<Server>(8800);
    std::shared_ptr<goe::Charger> goeCharger = std::make_shared<goe::Charger>("goeCharger", "192.168.178.106");
    server->goeCharger = goeCharger;

    std::thread server_thread(server_thread_func, server);

    while(run_servers){
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }

    std::cout << "closing server..." << std::endl;

    server->stop();

    server_thread.join();

    return 0;
}
